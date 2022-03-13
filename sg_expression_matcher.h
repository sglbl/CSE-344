#ifndef _EXPRESSIONMATCHER_H_
#define _EXPRESSIONMATCHER_H_

/*
	@author Suleyman Golbol
	@number 1801042656
*/

/* Returns the size of string */
int sg_strlen(char* string);
/* My implementation for strncat. The difference is in sg_ function, it doesn't add to str1, instead it returns */
char* sg_strcat(char* str1, char* str2);
/* My implementation for strcat. Concatanates str1 and str2 and returns */
char* sg_strncat(char* str1, char* str2, int n);
/* My implementation for strncat. Concatanates str1 and str2 and returns */
int sg_strncmp(char* str1, char* str2, int n);
/* My implementation for strcmp. The difference is in sg_ function, it compares boolean way. Returns 0 if matches and -1 if not */
int sg_strcmp(char* str1, char* str2);

void sg_strcpy(char* dest, char* source);

void sg_strncpy(char* dest, char* source, int n);

#endif