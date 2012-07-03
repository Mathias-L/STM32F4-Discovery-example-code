#include <stdlib.h>

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
#define ADC_GRP2_NUM_CHANNELS   10
#define ADC_GRP2_BUF_DEPTH      1024
static adcsample_t samples2[ADC_GRP2_NUM_CHANNELS * ADC_GRP2_BUF_DEPTH];


/*
 * Internal Reference Voltage, according to ST this is 1.21V typical
 * with -40°C<T<+105°C its Min: 1.18V, Typ 1.21V, Max: 1.24V
 */
#define VREFINT 121
/*
 * The measured Value is initialized to 2^16/3V*2.21V
 */
uint32_t VREFMeasured = 26433;

/*
 * second storage ring buffer for continuous scan
 */
#define BUFFLEN    1024
unsigned int p1=0,p2=0;
unsigned int overflow=0;
unsigned long data[BUFFLEN];
unsigned long vref[BUFFLEN];
unsigned long temp[BUFFLEN];

/*
 * Error callback, does nothing
 */
static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {

  (void)adcp;
  (void)err;
  if(running){
    data[p1++]=0;
    overflow++;
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

 /*
  * measures ADC_GRP1_BUF_DEPTH samples and displays all of them
  */
void cmd_measureDirect(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argv;
  unsigned int i;
  if(running){
    chprintf(chp, "Continuous measurement already running\r\n");
    return;
  }
  if (argc >0 ) {
    chprintf(chp, "Usage: measure\r\n");
    return;
  }
  adcConvert(&ADCD1, &adcgrpcfg1, samples1, ADC_GRP1_BUF_DEPTH);
  chprintf(chp, "Measured:  ");
  for (i=0;i<ADC_GRP1_BUF_DEPTH;i++){
      chprintf(chp, "%d  ", samples1[i]);
  }
  chprintf(chp, "\r\n");
}

 /*
  * prints the last measured Temperature from measureContinuous
  * According to ST:
  *   2.5 mV/°C
  *   25°C === 0.76V
  * Thus:
  *  temp [°C] = (temp[V]-0,76 [V])/0,0025 [V/°C] +25 [°C]
  */
void cmd_Temperature(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argv;
  int thisTemp;
  if (argc >0 ) {
    chprintf(chp, "Usage: temp\r\n");
    return;
  }
  if(!running){
    chprintf(chp, "No Background conversion running\r\n");
    return;
  }
  //Get the last used pointer
  int myp1 =  (p1-1+BUFFLEN)%BUFFLEN;
  //Convert to Voltage and then to °C
  thisTemp = (((int64_t)temp[myp1])*VREFINT*400/VREFMeasured-30400)+2500;
  chprintf(chp, "Temperatur: %d.%2U°C\r\n", thisTemp/100,thisTemp%100);
  //temp[p1];
}

 /*
  * prints and sets the measured value for VREFint
  */
void cmd_Vref(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argv;
  if (argc >1 ) {
    chprintf(chp, "Usage: vref [newValue]\r\n");
    return;
  }
  chprintf(chp, "VREFmeasured: %U\r\n", VREFMeasured);
  chprintf(chp, "VREFInt: %U.%2UV\r\n", VREFINT/100,VREFINT%100);
  if(argc==1){
    VREFMeasured = atoi(argv[0]);
  }
}

 /*
  * averages ADC_GRP1_BUF_DEPTH samples and converts to analog voltage
  */
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

  /*
   * Conversion to 1/10mV: Max Value exuals ~3V
   *  This is no proper calibration!!
   *  The uint64_t cast prevents overflows
   */
  //Using 2^16 === 3V
  //sum = ((uint64_t)sum)/(ADC_GRP1_BUF_DEPTH/16)*30000/65536;
  //sum = (((uint64_t)sum)*1875)>>22;  //This is the inlined version of the above

  //Using VREFMeasured === VREFINT
  //sum = ((uint64_t)sum)/(ADC_GRP1_BUF_DEPTH/16)*VREFINT/VREFMeasured;
  sum = ((uint64_t)sum)*VREFINT/(ADC_GRP1_BUF_DEPTH/16*VREFMeasured/100);

  //prints the averaged value with 4 digits precision
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

  unsigned int i,j;
  uint32_t sum=0;
  uint32_t vrefSum=0;
  uint32_t tempSum=0;
  if(n != ADC_GRP2_BUF_DEPTH/2) overflow++;
  for(i=0;i<ADC_GRP2_BUF_DEPTH/2;i++){
    for (j=0;j<8;j++){
      sum+=buffer[i*ADC_GRP2_NUM_CHANNELS+j];
    }
    vrefSum +=buffer[i*ADC_GRP2_NUM_CHANNELS+8];
    tempSum +=buffer[i*ADC_GRP2_NUM_CHANNELS+9];
  }
  vref[p1] = vrefSum/(ADC_GRP2_BUF_DEPTH/4/8);
  temp[p1] = tempSum/(ADC_GRP2_BUF_DEPTH/4/8);
  data[p1] = sum/(ADC_GRP2_BUF_DEPTH/4);

  // Only propagate 1/4th of the measured value to average VREF further
  VREFMeasured = (VREFMeasured*3+vref[p1])>>2;
  ++p1;
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
  ADC_SMPR1_SMP_AN12(ADC_SAMPLE_480) | ADC_SMPR1_SMP_AN11(ADC_SAMPLE_480) |
  ADC_SMPR1_SMP_SENSOR(ADC_SAMPLE_480) | ADC_SMPR1_SMP_VREF(ADC_SAMPLE_480),  //sample times ch10-18
  0,                                                                        //sample times ch0-9
  ADC_SQR1_NUM_CH(ADC_GRP2_NUM_CHANNELS),                                   //SQR1: Conversion group sequence 13...16 + sequence length
//  ADC_SQR2_SQ8_N(ADC_CHANNEL_IN11)   | ADC_SQR2_SQ7_N(ADC_CHANNEL_IN11),    //SQR2: Conversion group sequence 7...12
  ADC_SQR2_SQ10_N(ADC_CHANNEL_SENSOR) | ADC_SQR2_SQ9_N(ADC_CHANNEL_VREFINT) |
  ADC_SQR2_SQ8_N(ADC_CHANNEL_IN11)   | ADC_SQR2_SQ7_N(ADC_CHANNEL_IN11) ,
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
    chprintf(chp, "%U:%U-%U-%U  ", p2, data[p2], vref[p2], temp[p2]);
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
  palSetGroupMode(GPIOC, PAL_PORT_BIT(1),
                  0, PAL_MODE_INPUT_ANALOG);
  adcStart(&ADCD1, NULL);
  //enable temperature sensor and Vref
  adcSTM32EnableTSVREFE();
}
