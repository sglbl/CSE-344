#ifndef ADDITIONAL_H_
#define ADDITIONAL_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

#define TRUE 1
#define FALSE 0

typedef struct Info {
    int index;
    int fileDescs[3];
} Info;

int *openFiles(char *filePath1, char *filePath2, char *output);

void readMatrices(int n, int m, int twoToN, int fileDescs[3], char matrixA[][twoToN], char matrixB[][twoToN]);

/* Creates threads */
void createThreads(int twoToN, char matrixA[twoToN][twoToN], char matrixB[twoToN][twoToN]);
/* Thread routine */
void *threadJob(void *arg);
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