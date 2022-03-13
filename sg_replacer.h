#ifndef _MYHEADER_H_
#define _MYHEADER_H_
/*
	@author Suleyman Golbol
	@number 1801042656
*/

/* Struct to hold info of string to change [both index and value] */
typedef struct StrInfo{
	int index;
	char* str;
}StrInfo;

typedef enum ReplaceMode{   
    NORMAL,
    INSENSTIVE,
    MULTIPLE,
    LINE_START,
    LINE_END,
    REPETITION,
    ARBITRARY
} ReplaceMode;

/* Dividing argv[1] into different operations as 2D array / double pointer */
char** argDivider(char* arg, int *counter);

/* Holds necessary loop to parse operation into 2 strings and replace values */
void exchanger(char* buffer, char** operations, int size);

/* Prints error and instruction about how to run app */
void printErrorAndExit();

/* Replaces value of str1 and str2 in buffer and writes back to buffer [using ReplaceMode] */
void replace(char* buffer, char *str1, char *str2, ReplaceMode mode);

/* Returns bigger value from @param indexOfStr1 and @param indexOfStr2 */
int biggerReturner(int indexOfStr1, int indexOfStr2);

#endif