
#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "myADC.h"



/*
 * Global lock for the ADC.
 * I should probably use proper locking mechanisms provided by chibios!
 */
int running=0;


/*
 * Defines for single scan conversion
 */
#define ADC_GRP1_NUM_CHANNELS   1
#define ADC_GRP1_BUF_DEPTH      2048*2*4

/*
 * Buffer for single conversion
 */
static adcsample_t samples1[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];

/*
 * Defines for continuous scan conversions
 */
#define ADC_GRP2_NUM_CHANNELS   8
#define ADC_GRP2_BUF_DEPTH      1024
static adcsample_t samples2[ADC_GRP2_NUM_CHANNELS * ADC_GRP2_BUF_DEPTH];

/*
 * second storage ring buffer for continuous scan
 */
#define BUFFLEN    1024
unsigned int p1=0,p2=0;
unsigned int overflow=0;
unsigned long data[BUFFLEN];

/*
 * Error callback, does nothing
 */
static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {

  (void)adcp;
  (void)err;
  if(running){
    data[p1++]=0;
  }
}

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 8 samples of 1 channel, SW triggered.
 * Channels:    IN11.
 */
static const ADCConversionGroup adcgrpcfg1 = {
  FALSE,                        //circular buffer mode
  ADC_GRP1_NUM_CHANNELS,        //Number of the analog channels
  NULL,                         //Callback function (not needed here)
  adcerrorcallback,             //Error callback
  0,                                        /* CR1 */
  ADC_CR2_SWSTART,                          /* CR2 */
  ADC_SMPR1_SMP_AN11(ADC_SAMPLE_3),         //sample times ch10-18
  0,                                        //sample times ch0-9
  ADC_SQR1_NUM_CH(ADC_GRP1_NUM_CHANNELS),   //SQR1: Conversion group sequence 13...16 + sequence length
  0,                                        //SQR2: Conversion group sequence 7...12
  ADC_SQR3_SQ1_N(ADC_CHANNEL_IN11)          //SQR3: Conversion group sequence 1...6
};

/*
 * console invocatable function for a single analog conversion
 * converts ADC_GRP1_BUF_DEPTH samples and averages them
 * with ADC_GRP1_BUF_DEPTH==2048 this gives around 14 bit precision
 * even though the ADC hardware has only 12 bits internal precision
 * also at 2048 samples the 14 bit are nearly noise free, while
 * each sample is very noisy.
 * WARNING: If you average to many samples, the variable containing
 *  the sum may overflow. That means that your readings will be wrong.
 * However: With the ADC delivering a max value of 2^12, with uint32_t
 *  you can average 2^20 = 1048576 samples.
 */
void cmd_measure(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argv;
  uint32_t sum=0;
  unsigned int i;
  if(running){
    chprintf(chp, "Continuous measurement already running\r\n");
    return;
  }
  if (argc >0 ) {
    chprintf(chp, "Usage: measure\r\n");
    return;
  }
  //for(i=0;i<160;i++)
  adcConvert(&ADCD1, &adcgrpcfg1, samples1, ADC_GRP1_BUF_DEPTH);
  //prints the first measured value
  chprintf(chp, "Measured: %d  ", samples1[0]*16);
  sum=0;
  for (i=0;i<ADC_GRP1_BUF_DEPTH;i++){
      //chprintf(chp, "%d  ", samples1[i]);
      sum += samples1[i];
  }
  //prints the averaged value with two digits precision
  chprintf(chp, "%U\r\n", sum/(ADC_GRP1_BUF_DEPTH/16));
}

void cmd_measureA(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argv;
  uint32_t sum=0;
  unsigned int i;
  if(running){
    chprintf(chp, "Continuous measurement already running\r\n");
    return;
  }
  if (argc >0 ) {
    chprintf(chp, "Usage: measure\r\n");
    return;
  }
  //for(i=0;i<160;i++)
  adcConvert(&ADCD1, &adcgrpcfg1, samples1, ADC_GRP1_BUF_DEPTH);
  sum=0;
  for (i=0;i<ADC_GRP1_BUF_DEPTH;i++){
      //chprintf(chp, "%d  ", samples1[i]);
      sum += samples1[i];
  }

  //Conversion to mV: Max Value exuals ~3V
  // This is no proper calibration!
  // The uint64_t cast prevents overflows
  //sum = ((uint64_t)sum)/(ADC_GRP1_BUF_DEPTH/16)*30000/65536;
  sum = ((uint64_t)sum)*1875/4194304;  //This is the inlined version of the above
  //prints the averaged value with two digits precision
  chprintf(chp, "Measured: %U.%04UV\r\n", sum/10000, sum%10000);
}


/*
 * This callback is called everytime the buffer is filled or half-filled
 * A second ring buffer is used to store the averaged data.
 * I should use a third buffer to store a timestamp when the buffer was filled.
 * I hope I understood how the Conversion ring buffer works...
 */

static void adccallback(ADCDriver *adcp, adcsample_t *buffer, size_t n) {

  (void)adcp;
  (void)n;

  unsigned int i;
  uint32_t sum=0;
  if (samples2 == buffer) {
    for(i=0;i<ADC_GRP2_BUF_DEPTH*ADC_GRP2_NUM_CHANNELS/2;i++){
      sum+=buffer[i];
    }
    data[p1++] = sum/(ADC_GRP2_BUF_DEPTH/4);
  }
  else {
    for(i=0;i<ADC_GRP2_BUF_DEPTH*ADC_GRP2_NUM_CHANNELS/2;i++){
      sum+=buffer[i];
    }
    data[p1++] = sum/(ADC_GRP2_BUF_DEPTH/4);
  }
  p1 = p1%BUFFLEN;
  if(p1==p2) ++overflow;
}


static const ADCConversionGroup adcgrpcfg2 = {
  TRUE,                     //circular buffer mode
  ADC_GRP2_NUM_CHANNELS,    //Number of the analog channels
  adccallback,              //Callback function
  adcerrorcallback,         //Error callback
  0,                        /* CR1 */
  ADC_CR2_SWSTART,          /* CR2 */
  ADC_SMPR1_SMP_AN12(ADC_SAMPLE_480) | ADC_SMPR1_SMP_AN11(ADC_SAMPLE_480),  //sample times ch10-18
  0,                                                                        //sample times ch0-9
  ADC_SQR1_NUM_CH(ADC_GRP2_NUM_CHANNELS),                                   //SQR1: Conversion group sequence 13...16 + sequence length
  ADC_SQR2_SQ8_N(ADC_CHANNEL_IN11)   | ADC_SQR2_SQ7_N(ADC_CHANNEL_IN11),    //SQR2: Conversion group sequence 7...12
  ADC_SQR3_SQ6_N(ADC_CHANNEL_IN11)   | ADC_SQR3_SQ5_N(ADC_CHANNEL_IN11) |
  ADC_SQR3_SQ4_N(ADC_CHANNEL_IN11)   | ADC_SQR3_SQ3_N(ADC_CHANNEL_IN11) |
  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN11)   | ADC_SQR3_SQ1_N(ADC_CHANNEL_IN11)     //SQR3: Conversion group sequence 1...6
};

/*
 * Start a continuous conversion
 */
void cmd_measureCont(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)chp;
  (void)argc;
  (void)argv;
  if(running){
    chprintf(chp, "Continuous measurement already running\r\n");
  }else {
    running=1;
    adcStartConversion(&ADCD1, &adcgrpcfg2, samples2, ADC_GRP2_BUF_DEPTH);
  }
}

/*
 * Stop a continuous conversion
 */
void cmd_measureStop(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)chp;
  (void)argc;
  (void)argv;
  if(running){
    adcStopConversion(&ADCD1);
    running=0;
  }
}

/*
 * print the remainder of the ring buffer of a continuous conversion
 */
void cmd_measureRead(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)chp;
  (void)argc;
  (void)argv;
  while(p1!=p2){
    chprintf(chp, "%U:%U  ", p2, data[p2]);
    if (data[p2]==0){
      chprintf(chp, "\r\n Error!\r\n  ", p2, data[p2]);
    }
    p2 = (p2+1)%BUFFLEN;
  }
  chprintf(chp, "\r\n");
  if(overflow){
    chprintf(chp, "Overflow: %U  \r\n", overflow);
    overflow=0;
  }
}

void myADCinit(void){
  palSetGroupMode(GPIOC, PAL_PORT_BIT(1) | PAL_PORT_BIT(2),
                  0, PAL_MODE_INPUT_ANALOG);
  adcStart(&ADCD1, NULL);
  //enable temperature sensor
  //adcSTM32EnableTSVREFE();
}
