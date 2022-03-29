#ifndef SGPROCESSP_H_
#define SGPROCESSP_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/
#define TRUE 1
#define FALSE 0
#define CHILD_SIZE 10
#define COORD_DIMENSIONS 3

/* Prints error and instruction about how to run app */
void printUsageAndExit();

/* Reads file with using fileDescriptor; use fork and use waitpid to make sure parent process waits for child process to finish */
void reader(int fileDescriptor, char *argv[], int fileSize);

/* Creates child process and calls execve for it */
void spawnChild(char *fileName , int i, char **buffer);

/* Signal handler in case of SIGINT command be catched. */
void sg_signalHandler(int signalNumber);

/* Killing the child process */
void killTheKidsAndParent(int fileDescriptor, char *argv[]);

/* Frees buffer */
void freeingBuffer(char **buffer, int size);

/* Collects output from children as binary and stores as double** */
void collectOutputFromChildren(char *filePath);

/* Calculates Frobenius Norm */
void calcFrobeniusNorm(double **output, int fileSize);

/* Cleans the output file and if doesn't exist->creates. */
void cleanTheOutputFile(char *argv[]);

/* Checks if the char is non-ascii */
void checkIfNonAscii(char character);

/* Converts ascii non-negative integer to string value */
char* itoaForAscii(int number);

/* Double to string for basic nonnegative distance values. */
char* basicFtoa(double number);

/* Returns the size of string */
int sg_strlen(char* string);

#endif