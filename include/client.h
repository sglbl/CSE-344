#ifndef CLIENT_H_
#define CLIENT_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

/* Opens files */
void openFiles(char *filePath1, char *filePath2, char *output, int fileDescs[3]);
/* Client routine */
void doClientJob(char *filePath, int portNo, char *ipv4);
/* Initializes signal handler */
void signalHandlerInitializer();
/* Signal handler */
void mySignalHandler(int signalNumber);
/* Barrier implementation */
void barrier();

#endif