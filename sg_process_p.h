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

/* Reads file with using fileDescriptor */
void reader(int fileDescriptor, char *argv[], int fileSize);

/* Creates child process and uses waitpid to make sure parent process waits for child process to finish */
void spawn(char *argv[], char **buffer, int i);

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