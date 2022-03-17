#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include "sg_replacer.h"
#define TRUE 1
#define FALSE 0

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
    write(1 /* File descriptor for stdout */, "Goodbye!\n", 10);
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

    char** operations = (char**)calloc(numberOfOperations, sizeof(char*));
    char *token = strtok(arg, ";");
    for(i = 0; token != NULL && i < numberOfOperations; i++){
        operations[i] = (char*)calloc(30, sizeof(char));
        operations[i] = token;
        token = strtok(NULL, ";");
    }
    *counter = i;
    
    return operations;
}

void replacer(char* buffer, char** operations, int size){
    // For every operation (that's divided by / symbols on argument )
    for(int i=0; i<size; i++){    
        char* tempOperation = (char*)calloc(strlen(operations[i]), sizeof(char));
        strncpy(tempOperation, operations[i], strlen(operations[i]));                                 
        ReplaceMode mode = SENSITIVE;

        // Parsing operations[i] to get the first argument of operation
        char *str1 = strtok(tempOperation, "/");
        // Parsing operations[i] again to get the second argument of operation
        char *str2 = strtok(NULL, "/");
        // Check if argumant contains 'i', if contains it's insensitive
        if( strtok(NULL, "/") != NULL ){
            mode = INSENSITIVE;
        }
        
        // This function uses bitwise | operator in order to select replace mode in an easier way.
        if(str1[0] == '^'){        // if first argumant after / has ^ then it means it should support matching at line starts
            mode |= LINE_START;
            str1++; //Moving string one char right so get rid of ^
        }
        if(str1[strlen(str1) - 1] == '$'){  // if first argumant has $ then it means it should support matching at line ends
            mode |= LINE_END;
            str1[ strlen(str1) - 1 ] = '\0'; // Truncate argument by 1 [Removing $ sign from the end]
        }
        
        if( strchr(str1, '*') != NULL ){  // If argument is "st*r9"
            mode |= REPETITION;
        }

        // If operation contains [ and ] characters than it supports multiple 
        if(strchr(operations[i], '[') != NULL && strchr(operations[i], ']') != NULL && ((mode & REPETITION) == 0) ){
            multipleReplacer(buffer, ++operations[i], mode);  // Changing cursor 1 to the right so get rid of '/' symbol
            continue;
        }

        replace(buffer, str1, str2, mode);                         // Replacing
    }

}

void replace(char* buffer, char *str1, char *str2, ReplaceMode mode){
    int foundFlag = FALSE;
    int bufferSize = strlen(buffer);

    StrInfo str1Info; //To hold size and index 

    // Getting index of str1 (argument 1) to replace
    for(int i=0; i<bufferSize; i++){
        str1Info.index = -1;
        if( mode == SENSITIVE ){
            if( strncmp(buffer+i, str1, strlen(str1) ) == 0){
                str1Info.index = i;
                str1Info.size = strlen(str1);
                foundFlag = TRUE;
            }
        }
        else if( mode == INSENSITIVE ){
            if( strncasecmp(buffer+i, str1, strlen(str1) ) == 0){
                str1Info.index = i;
                str1Info.size = strlen(str1);
                foundFlag = TRUE;
            }
        }
        else if( mode == (SENSITIVE | LINE_START) ){
            if( (buffer+i-1)[0] == '\n' && strncmp(buffer+i, str1, strlen(str1) ) == 0){ //Checking if starting of a line
                str1Info.index = i;
                str1Info.size = strlen(str1);
                foundFlag = TRUE;
            }
        }
        else if( mode == (INSENSITIVE | LINE_START) ){
            if( ( (buffer+i-1)[0] == '\n' || i == 0)  // If it's first line; then it means that it's first line of file or the previous char is '\n'
                && strncasecmp(buffer+i, str1, strlen(str1) ) == 0){ //Checking if starting of a line
                str1Info.index = i;
                str1Info.size = strlen(str1);
                foundFlag = TRUE;
            }
        }
        else if( mode == (SENSITIVE | LINE_END) ){
            if( ( ((buffer + strlen(str1) + i)[0] == '\n') || ((buffer + strlen(str1) + i)[0] == '\0') ) // If next char is '\n' or EOF then it supports $
                && strncmp(buffer+i, str1, strlen(str1) ) == 0){ //Checking if starting of a line
                str1Info.index = i;
                str1Info.size = strlen(str1);
                foundFlag = TRUE;
            }
        }
        else if( mode == (INSENSITIVE | LINE_END) ){
            if( ( ((buffer + strlen(str1) + i)[0] == '\n') || ((buffer + strlen(str1) + i)[0] == '\0') || ((buffer + strlen(str1) + i)[0] == '\r')  ) // If next char is '\n' or '\r' or EOF then it supports $
                && strncasecmp(buffer+i, str1, strlen(str1) ) == 0){ //Checking if starting of a line
                str1Info.index = i;
                str1Info.size = strlen(str1);
                foundFlag = TRUE;
            }
        }

        else if( mode == (SENSITIVE | LINE_START | LINE_END) ){
            if( ( (buffer+i-1)[0] == '\n' || i == 0) // If it's first line; then it means that it's first line of file or the previous char is '\n' 
                && ( ((buffer + strlen(str1) + i)[0] == '\n') || ((buffer + strlen(str1) + i)[0] == '\0') ) // If next char is '\n' or EOF then it supports $
                && strncmp(buffer+i, str1, strlen(str1) ) == 0){ //Checking if starting of a line
                str1Info.index = i;
                str1Info.size = strlen(str1);
                foundFlag = TRUE;
            }
        }
        else if( mode == (INSENSITIVE | LINE_START | LINE_END) ){
            if( ( (buffer+i-1)[0] == '\n' || i == 0) // If it's first line; then it means that it's first line of file or the previous char is '\n' 
                && ( ((buffer + strlen(str1) + i)[0] == '\n') || ((buffer + strlen(str1) + i)[0] == '\0') ) // If next char is '\n' or EOF then it supports $
                && strncasecmp(buffer+i, str1, strlen(str1) ) == 0){ //Checking if starting of a line
                str1Info.index = i;
                str1Info.size = strlen(str1);
                foundFlag = TRUE;
            }
        }
        else if( (mode & REPETITION /* 00001 */ ) == 1 /* Binary: 00001 */ ) { //If mode contains REPETITION (00001) then when we AND with 00001 it should give us 1.
            if(strchr(str1, '[') != NULL && strchr(str1, ']') != NULL){
                int leftSqIndex;
                for(leftSqIndex=0; str1[leftSqIndex] != '['; ++leftSqIndex);
                if( strncasecmp(str1, buffer + i, leftSqIndex) == 0 )
                    repetitionReplacerWithBracket(buffer, i, str1, str2, mode);
                continue;
            }

            char* strAfterKeyValue = strchr(str1, '*');     // FOR EXAMPLE if str1 argument is "st*r9" ; strAfterKeyValue is "*r9"
            char keyValue = (strAfterKeyValue-1)[0];        // Key value is in 1 backward than '*' so it's 't'
            char* strBeforeKeyValue = (char*)calloc(strlen(str1) - strlen(strAfterKeyValue) - 1 , sizeof(char) );
            strncpy(strBeforeKeyValue, str1, strlen(str1) - strlen(strAfterKeyValue) - 1); // When we substitute we find strBeforeKeyValue size. 
            strAfterKeyValue++;                             // Moving cursor 1 right to remove * sign from after key value. Now strAfterKeyValue is "r9"
            
            if( strncasecmp(buffer+i, strBeforeKeyValue, strlen(strBeforeKeyValue) ) == 0 ){ //If string before key value matches enter repetitionReplacer() to check more.
                repetitionReplacer(buffer, i, strBeforeKeyValue, keyValue, strAfterKeyValue, str2, TRUE, mode );
            }
            continue;
        }

        if(str1Info.index != -1){
            foundFlag = TRUE;
            int difference = strlen(str2) - str1Info.size;

            char* tempString = (char*)calloc( strlen(buffer) + abs(difference) , sizeof(char) );
            strncpy( tempString, buffer, i );
            strncat( tempString, str2, strlen(str2) );   // Adding new string to replace to tempString
            strcat( tempString, buffer + i + strlen(str1) ); // Moving cursor(iter) 2 left to show the after appended string.

            for(int j=0; j < strlen(tempString); j++)
                buffer[j] = tempString[j];
            for(int j=0; j< bufferSize - strlen(tempString); j++)
                buffer[j + strlen(tempString)] = '\0';
            buffer[strlen(tempString)] = '\0';

        }
        
    }

    if(foundFlag == FALSE && (mode & REPETITION /* 00001 */ ) != 1 ){
        write(2 /* file descriptor of stderr */ , "WARNING! One or more arguments you want to replace couldn't be found on file.\n", 79);
        return;
    }
}

void repetitionReplacerWithBracket(char *buffer, int i, char *str1, char *str2, ReplaceMode mode){
    int isLast = FALSE; // This will be used for the square bracket condition like Window[sz]* . 
                // In loop, if it's the last char before ']', we can change 0 repetition.
    int leftSqIndex, rightSqIndex;

    for(leftSqIndex=0; str1[leftSqIndex] != '['; ++leftSqIndex); 
    for(rightSqIndex=leftSqIndex; str1[rightSqIndex] != ']'; ++rightSqIndex); 

    char* strAfterKeyValue = strchr(str1, '*');     // FOR EXAMPLE if str1 argument is "st*r9" ; strAfterKeyValue is "*r9"
    char* strBeforeKeyValue = (char*)calloc(leftSqIndex , sizeof(char) );
    strncpy(strBeforeKeyValue, str1, leftSqIndex ); // When we substitute we find strBeforeKeyValue size. 
    strAfterKeyValue++;                             // Moving cursor 1 right to remove * sign from after key value. Now strAfterKeyValue is "r9"

    for(int j = leftSqIndex + 1; j < rightSqIndex; j++){
        if(j == rightSqIndex - 1)
            isLast = TRUE;
        repetitionReplacer(buffer, i, strBeforeKeyValue, str1[j], strAfterKeyValue, str2, isLast, mode);
    }

}

void repetitionReplacer(char *buffer, int i, char *strBeforeKeyValue, char keyValue, char *strAfterKeyValue, char *str2, int isLast, ReplaceMode mode){
    int repetitionCounter = 0;  // Counting how many repetition are there
    int size = strlen(strBeforeKeyValue) + strlen(strAfterKeyValue); // Size of string if has 0 repetition
    char* str1; // String that will be replaced

    // FOR EXAMPLE : ARGUMENT IS "st*r9"
    for(int j = 0; j < size; j++){
        if( strncasecmp(buffer + i + strlen(strBeforeKeyValue) + j , &keyValue, 1 ) == 0){  // Check how many times does it repeat
            repetitionCounter++;
        }
        else
            break;
    }
    if(repetitionCounter == 0){ // If 0 times repeating than string not found; because if it was zero we would have already found above.
        char* concatWith0Repetition = (char*)calloc(size, sizeof(char));
        strcpy(concatWith0Repetition, strBeforeKeyValue);
        strcat(concatWith0Repetition, strAfterKeyValue);
        // CONDITION : 0 repetition. Checking "sr9"
        if(isLast != TRUE)
            return; //WHat if other repetition condition has value with repetitions
        if( (mode & SENSITIVE) != 0 && strncmp(buffer + i, concatWith0Repetition, size) == 0){ //If sensitive and string matches
            str1 = concatWith0Repetition;
        }
        else if( (mode & INSENSITIVE) != 0 && strncasecmp(buffer + i, concatWith0Repetition, size) == 0){ //If not sensitive and string matches
            str1 = concatWith0Repetition;
        }
        else 
            return; 
    }
    else if(repetitionCounter != 0){    
   
        if( (mode & SENSITIVE) != 0 ){
            if( strncmp(buffer + i + strlen(strBeforeKeyValue) + repetitionCounter , strAfterKeyValue, strlen(strAfterKeyValue) ) == 0 ){
                // String found.  For "st*r9" --> "s" + "t (repetitionCounter Times)" + "r9"
                str1 = (char*)calloc( strlen(strBeforeKeyValue) + repetitionCounter + strlen(strAfterKeyValue), sizeof(char) );
                strcpy(str1, strBeforeKeyValue);
                for(int j = 0; j < repetitionCounter; j++)
                    strncat(str1, &keyValue, 1);  // Concatanate keyValue "repetitionCounter" times.
                strcat(str1, strAfterKeyValue);
            }
            else
                return; // String couldn't be found.
        }
        else if( (mode & INSENSITIVE) != 0 ){
            if( strncasecmp(buffer + i + strlen(strBeforeKeyValue) + repetitionCounter , strAfterKeyValue, strlen(strAfterKeyValue) ) == 0 ){
                // String found.  For "st*r9" --> "s" + "t (repetitionCounter Times)" + "r9"
                str1 = (char*)calloc( strlen(strBeforeKeyValue) + repetitionCounter + strlen(strAfterKeyValue), sizeof(char) );
                strcpy(str1, strBeforeKeyValue);
                for(int j = 0; j < repetitionCounter; j++)
                    strncat(str1, &keyValue, 1);  // Concatanate keyValue "repetitionCounter" times.
                strcat(str1, strAfterKeyValue);
            }
            else
                return; // String couldn't be found.
            
        }
    }
    // Now it's time to replace str1 value with str2 in file.
    size += repetitionCounter;  // This is size of str1 that will be replaced by other string str2.
    int difference = strlen(str2) - size;
    
    if( (mode & LINE_START) != 0 ){ //then it means it contains ^
        if( i != 0 && (buffer+i-1)[0] != '\n'){ //Checking if starting of a line or the beginning of file.
            return;
        }
    }
    if( (mode & LINE_END) != 0 ){ //then it means it contains $
        if( ((buffer + strlen(str1) + i)[0] != '\n') && ((buffer + strlen(str1) + i)[0] != '\0') ) // If next char is '\n' or EOF then it supports $
            return;
    }

    char* tempString = (char*)calloc( strlen(buffer) + difference , sizeof(char) );
    strncpy( tempString, buffer, i );
    strncat( tempString, str2, strlen(str2) );   // Adding new string to replace to tempString
    strcat( tempString, buffer + i + strlen(str1) ); // Moving cursor(iter) 2 left to show the after appended string.

    for(int j=0; j < strlen(tempString); j++)
        buffer[j] = tempString[j];
    buffer[strlen(tempString)] = '\0';
    //free(tempString);
    
}

void multipleReplacer(char* buffer, char* operation, ReplaceMode mode){
    char* string;
    int leftSqIndex, rightSqIndex;
    
    char* arg1 = strtok(operation, "/");
    int size = strlen(arg1);
    char *str2 = strtok(NULL, "/");

    if(mode == (SENSITIVE | LINE_START) || mode == (SENSITIVE | LINE_START | LINE_END) || mode == (INSENSITIVE | LINE_START) || mode == (INSENSITIVE | LINE_START | LINE_END) )
        arg1++; // If it is a line start operation (^), then we need to increment it to shift right

    if(mode == (SENSITIVE | LINE_END) || mode == (SENSITIVE | LINE_START | LINE_END) || mode == (INSENSITIVE | LINE_END) || mode == (INSENSITIVE | LINE_START | LINE_END) )
        arg1[ strlen(arg1) - 1 ] = '\0'; // Truncate argument by 1 [Removing $ sign from the end]

    // Finding how many MULTIPLE operations are there.
    for(leftSqIndex=0; arg1[leftSqIndex] != '['; ++leftSqIndex); 
    for(rightSqIndex=leftSqIndex; arg1[rightSqIndex] != ']'; ++rightSqIndex); 

    for(int i = 1; i < (rightSqIndex - leftSqIndex); i++){
        string = (char*)calloc(size, sizeof(char));         // For example if first argument is S[TL]R1 
        strncat(string, arg1, leftSqIndex);            // Adding S to string  
        strncat(string, arg1 + leftSqIndex + i, 1);    // Adding T and L to string in different cycles of loop
        strncat(string, arg1 + rightSqIndex + 1, size - (rightSqIndex+1) ); // Adding R1 (adding rest of it to string).
        replace(buffer, string, str2, mode);
    }

}