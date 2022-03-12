#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "my_header.h"
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

char** argDivider(char* arg){
    if( strncmp(arg, "/", 1) != 0){ 
        printErrorAndExit();
    }

    //Finding number of operations
    int numberOfOperations = 1;
    for(int i = 0; i < strlen(arg); i++){
        if(arg[i] == ';') 
            numberOfOperations++;  
    }

    printf("Num of operations %d\n", numberOfOperations);

    char** operations = (char**)calloc(numberOfOperations, sizeof(char));
    
    char *token = strtok(arg, ";");
    for(int i = 0; token != NULL; i++){
        // printf( "Token-> %s\n", token ); //printing each token
        operations[i] = token;
        token = strtok(NULL, ";");
    }

    return operations;
}

void exchanger(char* buffer, char** operations, int size){
    for(int i=0; i<size; i++){                                     // For every operation (that's divided by / symbols on argument )
        char str1[STR_SIZE], str2[STR_SIZE], isI[2];               // Create string variables that will be use to replace
        sscanf(operations[i], "/%[^/]/%[^/]/%s", str1, str2, isI); // Parsing every operation into strings
        printf("1-> %s | 2-> %s | 3-> %s \n", str1, str2, isI);
        replace(buffer, str1, str2);                               // Replacing
    }

}

// Using temporary files
// void useTemp(){
//     int fdMkstemp;
//     //Creating temp file
//     char nameBuffer[] = "/tmp/tempFileBuffer-XXXXXX";
//     if( (fdMkstemp = mkstemp(nameBuffer) ) == -1 ){
//         perror("Error while opening the file.\n");
//         exit(0);
//     }
//     unlink(nameBuffer); //Unlinking so name will dissapear.
//     // OPERATIONS WITH TEMP FILE

//     if(close(fdMkstemp) == -1){
//         perror("Error while closing mkstemp file");
//         exit(0);
//     }
// }

void replace(char* buffer, char *str1, char *str2){
    int bufferSize = strlen(buffer);
    char *newString;

    int indexOfStr1 = -1, indexOfStr2 = -1;
    
    // Getting index numbers of strings to replace
    for(int i=0; i<bufferSize; i++){
        if( strncmp(buffer+i, str1, strlen(str1) ) == 0)
            indexOfStr1 = i;
        if( strncmp(buffer+i, str2, strlen(str2) ) == 0)
            indexOfStr2 = i;
    }

    if(indexOfStr1 == -1 || indexOfStr2 == -1){
        perror("One of the strings couldn't be found.\n");
        exit(0);
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
    /* a */ strncpy(newString, buffer, smaller.index);
    /* D */ strncat(newString, bigger.str, strlen(bigger.str) ); //Concatanating with str2.
    /* c */ strncat(newString, buffer + smaller.index + strlen(smaller.str),  bigger.index - (smaller.index + strlen(smaller.str)) );
    /* B */ strncat(newString, smaller.str, strlen(smaller.str) );
    /* e */ strcat(newString, buffer + bigger.index + strlen(bigger.str) );

    printf("New string is:\n'%s'\n", newString);
    strcpy(buffer, newString); //Changing buffer value that we passed on parameter by using strcpy()
}

int biggerReturner(int indexOfStr1, int indexOfStr2){
    if(indexOfStr1 >= indexOfStr2) 
        return indexOfStr1;
    return indexOfStr2;
}