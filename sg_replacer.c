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
        
        // If operation contains [ and ] characters than it supports multiple 
        if(strchr(operations[i], '[') != NULL && strchr(operations[i], ']') != NULL){
            multipleReplacer(buffer, ++operations[i] );  // Changing cursor 1 to the right so get rid of '/' symbol
            continue;
        }

        //printf("Operations[i] is %s\n", operations[i] );
        // Parsing operations[i] to get the first argument of operation
        char *str1 = strtok(operations[i], "/");
        // Parsing operations[i] again to get the second argument of operation
        char *str2 = strtok(NULL, "/");
        if( strtok(NULL, "/") == NULL )
            mode = NORMAL;
        else
            mode = INSENSITIVE; //that means it's 'i' which is insensitive

        printf("1-> %s | 2-> %s | 3-> %d \n", str1, str2, mode);

        if(str1[0] == '^'){
            mode = LINE_START;
            ++str1;
            printf("NEWW Str1 is %s\n", str1);
        }
        
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
            }
        }
        else if(mode == INSENSITIVE){
            if( strncasecmp(buffer+i, str1, strlen(str1) ) == 0){
                str1Info.index = i;
                str1Info.size = strlen(str1);
            }
        }
        else if(mode == LINE_START){
            if( (buffer+i-1)[0] == '\n' && strncmp(buffer+i, str1, strlen(str1) ) == 0){ //Checking if starting of a line
                str1Info.index = i;
                str1Info.size = strlen(str1);
                // printf("Str1 is %s\n", str1);
                // printf("Str2 is %s\n", str2);
            }
        }
        else if(mode == LINE_END){
            // ...
        }

        if(str1Info.index  != -1){
            for(int j=0; j < str1Info.size; j++)
                buffer[str1Info.index + j] = str2[j]; //Changing info in buffer
        }
        
    }

    if(str1Info.index  == -1){
        printf("WARNING! %s couldn't be found on file.\n", str1);
        return;
    }

    printf("New buffer is '%s'\n", buffer);
}

void multipleReplacer(char* buffer, char* operation){
    char* string;
    int leftSqIndex, rightSqIndex;
    
    ReplaceMode mode = NORMAL;
    if( operation[strlen(operation) - 1] == 'i' )
        mode = INSENSITIVE;

    char* arg1 = strtok(operation, "/");
    int size = strlen(arg1);
    char *str2 = strtok(NULL, "/");

    // Finding how many MULTIPLE operations are there.
    for(leftSqIndex=0; arg1[leftSqIndex] != '['; ++leftSqIndex); 
    for(rightSqIndex=leftSqIndex; arg1[rightSqIndex] != ']'; ++rightSqIndex); 

    for(int i = 1; i < (rightSqIndex - leftSqIndex); i++){
        string = (char*)calloc(size, sizeof(char));         // For example if first argument is S[TL]R1 
        strncat(string, operation, leftSqIndex);            // Adding S to string  
        strncat(string, operation + leftSqIndex + i, 1);    // Adding T and L to string in different cycles of loop
        strncat(string, operation + rightSqIndex + 1, size - (rightSqIndex+1) ); // Adding R1 (adding rest of it to string).
        replace(buffer, string, str2, mode);
    }

}