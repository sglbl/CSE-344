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
    int twoToN;
    int numOfColumnToCalculate;
    int **matrixA;
    int **matrixB;
} Info;

int *openFiles(char *filePath1, char *filePath2, char *output);

void readMatrices(int n, int m, int twoToN, int fileDescs[3], int matrixA[][twoToN], int matrixB[][twoToN]);
/* Creates threads */
void createThreads(int twoToN, int matrixA[twoToN][twoToN], int matrixB[twoToN][twoToN]);

void putMatrixToInfo(Info info, int twoToN, int matrixA[twoToN][twoToN], int matrixB[twoToN][twoToN]);

void matrixPrinter(int twoToN, int **matrix);

void barrier();

/* Thread routine */
void *threadJob(void *arg);
/* Prints with timestamp */
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