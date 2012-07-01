#include "ch.h"
#include "hal.h"
#include "chprintf.h"
#include <stdlib.h>

#include "myPWM.h"


/*
 * this PWM callback function gets called at the end of a period
 * usually it resets all used channels (to either high or low)
 */

static void pwmpcb(PWMDriver *pwmp) {

  (void)pwmp;
  palSetPad(GPIOD, GPIOD_LED4);
  palSetPad(GPIOD, GPIOD_LED5);
}

/*
 * PWM callback for channel 1 at the given duty cycle
 */
static void pwmc1cb(PWMDriver *pwmp) {

  (void)pwmp;
  palClearPad(GPIOD, GPIOD_LED4);
}

/*
 * PWM callback for channel 2 at the given duty cycle
 */
static void pwmc2cb(PWMDriver *pwmp) {

  (void)pwmp;
  palClearPad(GPIOD, GPIOD_LED5);
}

/*
 * PWM configuration
 * 2 of the 4 channels are used
 */
static PWMConfig pwmcfg = {
  1000000,                                  /* 1MHz PWM clock frequency.   */
  10000,                                    /* Initial PWM period 1/100S.       */
  pwmpcb,                                   /* callback gets triggered after each period */
  {
   {PWM_OUTPUT_ACTIVE_HIGH, pwmc1cb},       /* channel 1 callback at given duty cycle */
   {PWM_OUTPUT_ACTIVE_HIGH, pwmc2cb},       /* channel 2 callback at given duty cycle */
   {PWM_OUTPUT_DISABLED, NULL},             /* channel 1 callback unused */
   {PWM_OUTPUT_DISABLED, NULL}              /* channel 1 callback unused */
  },
  0,
};

/*
 * console callable function that sets a given duty cycle for channel 1
 * (the argument is in tics, so it makes sense to have it between 0 and 10000)
 */
void cmd_cycle(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argv;
  int cycle = 500;
  if (argc != 1) {
    chprintf(chp, "Usage: cycle duty\r\n");
    return;
  }
  cycle = atoi(argv[0]);
  chprintf(chp, "cycle: %d\r\n", cycle);
  //pwmEnableChannel(&PWMD2, 0, PWM_PERCENTAGE_TO_WIDTH(&PWMD2, atoi(argv[0])));
  pwmEnableChannel(&PWMD2, 0, atoi(argv[0]));
}


/*
 * console callable function that ramps channel zero with a given ramp
 * use it for instance with:
 * ch> ramp 0 10000 1000 1000
 * that creates a ramp that increases from 0 to 100% duty in 10 steps&secs
 */
void cmd_ramp(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argv;
  int from, to, step,i, delay = 100;
  if (argc < 3 || argc >4) {
    chprintf(chp, "Usage: ramp from to step [delay]\r\n");
    return;
  }
  if(argc == 4){
    delay = atoi(argv[3]);
  }
  from = atoi(argv[0]);
  to = atoi(argv[1]);
  step = atoi(argv[2]);
  for(i=from;i<to;i+=step){
      pwmEnableChannel(&PWMD2, 0, i);
      chThdSleepMilliseconds(delay);
  }

}

/*
 * starts the PWM device
 */
void mypwmInit(void){
    pwmStart(&PWMD2, &pwmcfg);
}
