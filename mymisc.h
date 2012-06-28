#ifndef MYMISC_H_INCLUDED
#define MYMISC_H_INCLUDED


void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_toggle(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_blinkspeed(BaseSequentialStream *chp, int argc, char *argv[]);

void startBlinker(void);


#endif // MYMISC_H_INCLUDED
