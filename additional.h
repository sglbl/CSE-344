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

void createThreads(int C, int N, char *path);

void *consumerThread(void *arg);

void *supplierThread(void *arg);

void postSemaphore(int semIndex);

void waitSemaphoreForBoth();

void tprintf(const char *restrict formattedStr, ...);

char *timeStamp();

void errorAndExit(char *errorMessage);


#endif