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

void createSemSet();

void createThreads(int cNumber, int nNumber, char *path);

void *consumerThread(void *arg);

void *supplierThread(void *arg);

void postSemaphore(int semIndex);

void waitSemaphoreForBoth(long consumerId);

void tprintf(const char *restrict formattedStr, ...);

void signalHandlerInitializer();

void mySignalHandler(int signalNumber);

char *timeStamp();

void errorAndExit(char *errorMessage);


#endif