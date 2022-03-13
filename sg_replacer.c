#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "sg_replacer.h"
#include "sg_expression_matcher.h"
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
    exit(0);
}

char** argDivider(char* arg, int *counter){
    int i;
    if( sg_strncmp(arg, "/", 1) != 0){ 
        printErrorAndExit();
    }

    //Finding number of operations
    int numberOfOperations = 1;
    for(int i = 0; i < sg_strlen(arg); i++){
        if(arg[i] == ';') 
            numberOfOperations++;  
    }

    char** operations = (char**)calloc(numberOfOperations, sizeof(char));
    
    char *token = strtok(arg, ";");
    for(i = 0; token != NULL; i++){
        // printf( "Token-> %s\n", token ); //printing each token
        operations[i] = token;
        token = strtok(NULL, ";");
    }
    *counter = i;

    return operations;
}

void exchangerSquBracket(char* buffer, char* operation, int size){
    printf("buffer is %s\n", buffer);
    printf("Op %s\n", operation );
    int counter;
    for(counter=0; operation[counter] != ']'; counter++);
    
    char** array = (char**)calloc(counter, sizeof(char*));
    for(int i=0; i<counter; i++){
        array[i][0] = operation[i];
        // for(int j=0; j<  )

        // strcpy(...);
            array[i][0] = operation[i];
    }
    

}

void exchanger(char* buffer, char** operations, int size){
    for(int i=0; i<size; i++){                                     // For every operation (that's divided by / symbols on argument )
        char str1[STR_SIZE], str2[STR_SIZE], isI[2];               // Create string variables that will be use to replace
        // if(operations[i][1] == '['){ // square brackets
        //     exchangerSquBracket(buffer, operations[i]+2, size);
        //     // sscanf(operations[i], "/[%s']'/%[^/]/%s", str1, str2, isI); // Parsing every operation into strings
        //     printf("NEWWW\n");
        //     printf("1-> %s | 2-> %s | 3-> %s \n", str1, str2, isI);
        // }

        sscanf(operations[i], "/%[^/]/%[^/]/%s", str1, str2, isI); // Parsing every operation into strings
        printf("1-> %s | 2-> %s | 3-> %s \n", str1, str2, isI);
        ReplaceMode mode = NORMAL;
        if( sg_strncmp(isI, "i", 1) == 0) mode = INSENSTIVE;
        printf("Mode is %d\n", mode);
        replace(buffer, str1, str2, mode);                         // Replacing
    }

}

void replace(char* buffer, char *str1, char *str2, ReplaceMode mode){
    int bufferSize = sg_strlen(buffer);
    char *newString;

    int indexOfStr1 = -1, indexOfStr2 = -1;
    
    // Getting index numbers of strings to replace
    for(int i=0; i<bufferSize; i++){
        if(mode == NORMAL){
            if( sg_strncmp(buffer+i, str1, sg_strlen(str1) ) == 0)
                indexOfStr1 = i;
            if( sg_strncmp(buffer+i, str2, sg_strlen(str2) ) == 0)
                indexOfStr2 = i;
        }
        else if(mode == INSENSTIVE){
            if( strncasecmp(buffer+i, str1, sg_strlen(str1) ) == 0) //ÇÇ
                indexOfStr1 = i;
            if( strncasecmp(buffer+i, str2, sg_strlen(str2) ) == 0)
                indexOfStr2 = i;
        }
    }

    if(indexOfStr1 == -1 || indexOfStr2 == -1){
        perror("One of the strings couldn't be found on arguments.\n");
        printErrorAndExit();
    }

    // Finding which index is bigger and which smaller because we need to know while we changing
    StrInfo bigger, smaller;
    if(biggerReturner(indexOfStr1, indexOfStr2) == indexOfStr2){
        bigger.index = indexOfStr2;
        bigger.str = str2;
        smaller.index = indexOfStr1;
        smaller.str = str1;
    }else{
        bigger.index = indexOfStr1;
        bigger.str = str1;
        smaller.index = indexOfStr2;
        smaller.str = str2;
    }

    // SWAPPING 
    // Let's say we have a text file which contains "a B c D e" and we want to replace B and D position into "newString"
    newString = (char*)calloc(bufferSize, sizeof(char));
    /* a */ sg_strncpy(newString, buffer, smaller.index);
    /* D */ newString = sg_strncat(newString, bigger.str, sg_strlen(bigger.str) ); //Concatanating with str2.
    /* c */ newString = sg_strncat(newString, buffer + smaller.index + sg_strlen(smaller.str),  bigger.index - (smaller.index + sg_strlen(smaller.str)) );
    /* B */ newString = sg_strncat(newString, smaller.str, sg_strlen(smaller.str) );
    /* e */ newString = sg_strcat(newString, buffer + bigger.index + sg_strlen(bigger.str) );

    printf("New string is:\n'%s'\n", newString);
    sg_strcpy(buffer, newString); //Changing buffer value that we passed on parameter by using strcpy()
}

int biggerReturner(int indexOfStr1, int indexOfStr2){
    if(indexOfStr1 >= indexOfStr2) 
        return indexOfStr1;
    return indexOfStr2;
}