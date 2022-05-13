#ifndef ADDITIONAL_H_
#define ADDITIONAL_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

#define TRUE 1
#define FALSE 0

typedef union SemUnion {
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
} SemUnion;

/* Creates semahpore set */
void createSemSet();
/* Creates threads */
void createThreads(int cNumber, int nNumber, char *path);
/* Consumer thread */
void *consumerThread(void *arg);
/* Supplier thread */
void *supplierThread(void *arg);
/* Posts semaphore */
void postSemaphore(int semIndex);
/* Waits for semaphore */
void waitSemaphoreForBoth(long consumerId);
/* tprintf */
void tprintf(const char *restrict formattedStr, ...);
/* Initializes signal handler */
void signalHandlerInitializer();
/* Signal handler */
void mySignalHandler(int signalNumber);
/* Returns time stamp */
char *timeStamp();
/* Prints error message and exits */
void errorAndExit(char *errorMessage);


#endif