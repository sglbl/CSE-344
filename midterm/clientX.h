#ifndef CLIENTX_H_
#define CLIENTX_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

void handleDataFile(char *dataFilePath, char *serverFifoPath);

int *matrixReader(char* dataFilePath, int *rowSize);

char* itoaForAscii(int number);

void signalHandlerInitializer();
// Create a signal handler function
void sg_signalHandler(int signalNumber);


void timePrinter();



#endif