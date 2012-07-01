
#include <stdlib.h>

#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "myMisc.h"


/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/


void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[]) {
  size_t n, size;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: mem\r\n");
    return;
  }
  n = chHeapStatus(NULL, &size);
  chprintf(chp, "core free memory : %u bytes\r\n", chCoreStatus());
  chprintf(chp, "heap fragments   : %u\r\n", n);
  chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const char *states[] = {THD_STATE_NAMES};
  Thread *tp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: threads\r\n");
    return;
  }
  chprintf(chp, "    addr    stack prio refs     state time\r\n");
  tp = chRegFirstThread();
  do {
    chprintf(chp, "%.8lx %.8lx %4lu %4lu %9s %lu\r\n",
            (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
            (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
            states[tp->p_state], (uint32_t)tp->p_time);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
}

void cmd_toggle(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argv;
  if (argc != 1) {
    chprintf(chp, "Usage: toggle #led\r\n");
    return;
  }
  if(argv[0][0]=='1'){
      palTogglePad(GPIOD, GPIOD_LED3);
  } else if(argv[0][0]=='2'){
      palTogglePad(GPIOD, GPIOD_LED4);
  } else if(argv[0][0]=='3'){
      palTogglePad(GPIOD, GPIOD_LED5);
  } else if(argv[0][0]=='4'){
      palTogglePad(GPIOD, GPIOD_LED6);
  }
}

int blinkspeed = 500;
void cmd_blinkspeed(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argv;
  int speed = 500;
  if (argc != 1) {
    chprintf(chp, "Usage: blinkspeed speed [ms]\r\n");
    return;
  }
  speed = atoi(argv[0]);
  if(speed>5000) speed = 5000;
  if(speed<5) speed = 5;
  blinkspeed = speed;
}

/*
 * Red LED blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palClearPad(GPIOD, GPIOD_LED6);
    chThdSleepMilliseconds(blinkspeed);
    palSetPad(GPIOD, GPIOD_LED6);
    chThdSleepMilliseconds(blinkspeed);

  }
  return 0;
}

void startBlinker(void){
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
}
