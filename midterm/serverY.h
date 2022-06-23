#ifndef SERVERX_H_
#define SERVERX_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

void handleClientRequest(char *logFilePath, int poolSizeY, int poolSizeZ, int sleepTime);

int checkIfMatrixIsInvertible(int *matrix, int rowSize, int logFileDesc);

long int determinantFinder(int *matrix, int rowSize);

long int cofactorFinder(int *matrix, int row, int column, int rowSize);

int daemonMaker();

void sg_perrorAndExit(char *errorMessage, int logFileDesc, int noExit);

void signalHandlerInitializer();

void sg_signalHandler(int signalNumber);

void serverZ(int logFileDesc, int yzPipeFileDesc[/*2*/], int poolSizeZ, int sleepTime);

char *itoaForAscii(int number);

char *timeStamp();

void timeStampPrinter(int fileDesc);

int singletonMakerAndCheckIfRunningAlready();

#endif