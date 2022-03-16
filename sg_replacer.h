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
    SENSITIVE = 16,  /* Binary: 10000 */
    INSENSITIVE = 8, /* Binary: 01000 */
    LINE_START = 4, /* Binary: 00100 */
    LINE_END = 2, /* Binary: 00010 */
    REPETITION = 1, /* Binary: 00001 */
} ReplaceMode; /* Replace mode is in binary mode actually because it will be used with bitwise | operations to check which conditions satisfy. */
//çç add to report about thinking more enum modes.

//* Prints error and instruction about how to run app */
void printErrorAndExit();

/* Dividing argv[1] into different operations as 2D array / double pointer */
char** argDivider(char* arg, int *counter);

/* Holds necessary loop to parse operation into 2 strings and replace values */
void replacer(char* buffer, char** operations, int size);

/* Replaces value of str1 and str2 in buffer and writes back to buffer [using ReplaceMode] */
void replace(char* buffer, char *str1, char *str2, ReplaceMode mode);

/* When program sees [ ] characters in argument then calls this function in order to replace multiple arguments */
void multipleReplacer(char* buffer, char* operation, ReplaceMode mode);

/* When program sees * character in argument then calls this function in order to replace argument with repetition */
void repetitionReplacer(char *buffer, int i, char *strBeforeKeyValue, char keyValue, char *strAfterKeyValue, char *str2, ReplaceMode mode);

/* When program sees *  and [ ] characters in argument then calls this function in order to replace argument with repetition */
void repetitionReplacerWithBracket(char *buffer, int i, char *str1, char *str2, ReplaceMode mode);

#endif