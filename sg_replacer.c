#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include "sg_replacer.h"
#define TRUE 1
#define FALSE 0
#define STR_SIZE 80

void printErrorAndExit(){
    perror( "ERROR FOUND ON ARGUMENTS; PLEASE ENTER A VALID INPUT! INSTRUCTIONS:\n"
    "./hw1 '/str1/str2/' inputFilePath -> For just to replace\n"
    "./hw1 '/str1/str2/i' inputFilePath ->Insensitive replace\n"
    "./hw1 '/str1/str2/i;/str3/str4/' inputFilePath -> Multiple replacement operations\n"
    "./hw1 '/[zs]tr1/str2/' inputFilePath -> Multiple character matching like this will match both ztr1 and str1\n"
    "./hw1 '/^str1/str2/' inputFilePath -> Support matching at line starts like this will only match lines starting with str1 \n"
    "./hw1 '/str1$/str2/' inputFilePath -> Support matching at line ends like this will only match lines ending with str1 \n"
    "./hw1 '/st*r1/str2/' inputFilePath -> Support 0 or more repetitions of characters like this will match sr1, str1, sttr1 \n"
    "./hw1 '/^Window[sz]*/Linux/i;/close[dD]$/open/' inputFilePath -> it supports arbitrary combinations of the above \n");
    printf("Goodbye!\n");
    exit(EXIT_FAILURE);
}

char** argDivider(char* arg, int *counter){
    int i;
    if( strncmp(arg, "/", 1) != 0){  // If first letter of argument is not '/' exit.
        printErrorAndExit();
    }

    //Finding number of operations
    int numberOfOperations = 1;
    for(i = 0; i < strlen(arg); i++){
        if(arg[i] == ';') 
            numberOfOperations++;  
    }

    char** operations = (char**)calloc(numberOfOperations, sizeof(char));
    char *token = strtok(arg, ";");
    for(i = 0; token != NULL; i++){
        operations[i] = token;
        token = strtok(NULL, ";");
    }
    *counter = i;

    return operations;
}

void replacer(char* buffer, char** operations, int size){
    // For every operation (that's divided by / symbols on argument )
    for(int i=0; i<size; i++){                                     
        ReplaceMode mode = NORMAL;
        int isInsensitive;
        
        // If operation contains [ and ] characters than it supports multiple 
        if(strchr(operations[i], '[') != NULL && strchr(operations[i], ']') != NULL){
            mode = MULTIPLE;
            squBracketReplacer(buffer, operations[i], size);
            return;
        }

        char *str1 = strtok(operations[i], "/");
        char *str2 = strtok(NULL, "/");
        if( strtok(NULL, "/") == NULL )
            isInsensitive = FALSE;
        else
            isInsensitive = TRUE; //that means it's 'i' which is insensitive

        printf("1-> %s | 2-> %s | 3-> %d \n", str1, str2, isInsensitive);
        
        if(isInsensitive == TRUE) mode = INSENSTIVE;
        printf("Mode is %d\n", mode);
        replace(buffer, str1, str2, mode);                         // Replacing
    }

}

void replace(char* buffer, char *str1, char *str2, ReplaceMode mode){
    int bufferSize = strlen(buffer);
    StrInfo str1Info; //To hold size and index 
    str1Info.index = -1;

    // Getting index of str1 (argument 1) to replace
    for(int i=0; i<bufferSize; i++){
        if(mode == NORMAL){
            if( strncmp(buffer+i, str1, strlen(str1) ) == 0){
                str1Info.index = i;
                str1Info.size = strlen(str1);
                printf("Str1 is %s\n", str1);
                printf("Str2 is %s\n", str2);
            }
        }
        else if(mode == INSENSTIVE){
            if( strncasecmp(buffer+i, str1, strlen(str1) ) == 0){
                str1Info.index = i;
                str1Info.size = strlen(str1);
            }
        }
    }

    if(str1Info.index  == -1){
        printf("WARNING! %s couldn't be found on file.\n", str1);
        return;
    }

    for(int i=0; i< str1Info.size; i++)
        buffer[str1Info.index + i] = str2[i]; //Changing info in buffer
    printf("New buffer is '%s'\n", buffer);
}

void squBracketReplacer(char* buffer, char* operation, int size){
    printf("buffer is '%s'\n", buffer);
    printf("Op %s\n", operation );
    int counter, counter2;

    for(counter=0; operation[counter] != '['; counter++); //Finding how many square bracket operations are there.
    for(counter2=0; operation[counter2] != ']'; counter2++);
    printf("Counter1  is %d\n", counter);
    printf("Counter2 is %d\n", counter2);

    char* stringAfterSqBracket = (char*)calloc(strlen(operation) - counter, sizeof(char));
    // "strlen(operation) - counter" shows the length between ] and /

    int i;
    for(i = counter; i < strlen(operation) && operation[i+1] != '/'; i++){
        stringAfterSqBracket[i - counter] = operation[i+1];
    } 
    i += 2; //Changing cursor to 2 forward.

    printf("Op[i] %c and op[i+1] %c\n", operation[i], operation[i+1] );

    // If string is [zs]tr1 then stringAfrerSqBracket = tr1
    // Finding str2
    char* str2 = (char*)calloc(strlen(operation) - i, sizeof(char));
    for(int j = 0; operation[i+j] != '/'; j++){
        str2[j] = operation[i+j];
    }
    printf("Str2 is %s\n", str2);

    char** array = (char**)calloc(counter, sizeof(char*));
    for(int i=0; i<counter; i++){
        array[i] = (char*)calloc(counter, sizeof(char));
        array[i][0] = operation[i];
        array[i] = strcat(array[i], stringAfterSqBracket); //For example concatanating z + tr1 && s + tr1
        printf("a[i] is %s\n" , array[i]);
        replace(buffer, array[i], str2, MULTIPLE);
    }

}