#ifndef SERVANT_H_
#define SERVANT_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

/* Servant routine */
void doServantJob();
/* Creates threads */
void createThreads(int portNo);
/* Thread routine */
void *threadJob(void *arg);
/* Initializes signal handler */
void signalHandlerInitializer();
/* Signal handler */
void mySignalHandler(int signalNumber);
/* Barrier implementation */
void barrier();

#endif