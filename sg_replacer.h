#ifndef SGREPLACER_H_
#define SGREPLACER_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

/* Struct to hold info of string to change [both index and value] */
typedef struct StrInfo{
	int index;
    int size;
}StrInfo;

typedef enum ReplaceMode{   
    NORMAL,
    INSENSITIVE,
    LINE_START,
    LINE_START_AND_LINE_END,
    INSENSITIVE_AND_LINE_START,
    INSENSITIVE_AND_LINE_END,
    INSENSITIVE_LINE_START_AND_LINE_AND,
    LINE_END,
    REPETITION,
} ReplaceMode;

/* Prints error and instruction about how to run app */
void printErrorAndExit();

/* Dividing argv[1] into different operations as 2D array / double pointer */
char** argDivider(char* arg, int *counter);

/* Holds necessary loop to parse operation into 2 strings and replace values */
void replacer(char* buffer, char** operations, int size);

/* Replaces value of str1 and str2 in buffer and writes back to buffer [using ReplaceMode] */
void replace(char* buffer, char *str1, char *str2, ReplaceMode mode);

void multipleReplacer(char* buffer, char* operation, ReplaceMode mode);

#endif