#ifndef PWM_H_INCLUDED
#define PWM_H_INCLUDED


void mypwmInit(void);
void cmd_ramp(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_cycle(BaseSequentialStream *chp, int argc, char *argv[]);

#endif // PWM_H_INCLUDED
