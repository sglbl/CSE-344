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
void spawn(char *argv[], char ***buffer);

void collectOutputFromChildren(char *filePath);

/* Cleans the output file and if doesn't exist->creates. */
void cleanTheOutputFile(char *argv[]);

/* Checks if the char is non-ascii */
void checkIfNonAscii(char character);

/* Returns the size of string */
int sg_strlen(char* string);

#endif