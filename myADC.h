#ifndef MYADC_H_INCLUDED
#define MYADC_H_INCLUDED

void cmd_measure(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_measureA(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_measureDirect(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_Vref(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_Temperature(BaseSequentialStream *chp, int argc, char *argv[]);

void cmd_measureCont(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_measureRead(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_measureStop(BaseSequentialStream *chp, int argc, char *argv[]);

void myADCinit(void);


#endif // MYADC_H_INCLUDED
