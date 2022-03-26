#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include "sg_process_p.h"

#define CHILD_SIZE 10
#define COORD_DIMENSIONS 3

void printErrorAndExit(){
    perror( "ERROR FOUND ON ARGUMENTS; PLEASE ENTER A VALID INPUT! INSTRUCTIONS:\n"
    "./processP -i inputFilePath -o outputFilePath\n");
    write(1 /* File descriptor for stdout */, "Goodbye!\n", 10);
    exit(EXIT_FAILURE);
}

void reader(int fileDescriptor, char *argv[]){
    int readedByte;
    int i = 1; //Name of the children will be R_i

    while(TRUE){
        // Allocating memory for buffer which will store the content of input file
        char **buffer = (char**)calloc( CHILD_SIZE + 1, sizeof(char*) );
        for(int j = 0; j < CHILD_SIZE; j++){

            buffer[j] = (char*)calloc( COORD_DIMENSIONS, sizeof(char*) );
            for(int k = 0; k < COORD_DIMENSIONS; k++){
                if( (readedByte = read(fileDescriptor, &buffer[j][k], 1 /* read 1 byte */)) > 0 ){
                    printf("Charbuffer is %c, and int value of it is %d\n", buffer[j][k], buffer[j][k]);
                }
                else if( readedByte <= 0 && errno == EINTR ){
                    perror("File reading error. Goodbye\n");
                    exit(EXIT_FAILURE);
                }
                else{
                    buffer[j] = NULL;
                    printf("File finished.\n");
                    return;
                }
                
            }
        }
        char iValue = i;
        spawn(argv, buffer, &iValue);
        i++;
    }    

    
}

int spawn(char** argv, char** buffer, char* i){
    pid_t pidCheckForChild = fork(); // Duplicating this process.
    if( pidCheckForChild != 0){
         // pidCheckForChild is not 0, so it's the parent process.
        printf("This is the parent.\n");
        // ÇÇ check for if all children returned, so we can calculate
        return pidCheckForChild;
    }
    else{ 
        // pidCheckForChild is 0 so we are in child process.
        printf("This is the child.\n");
        // char* iValueAsString = "i";
        // iValueAsString[0] = (char)i;
        printf("New i is %s\n", i);

        char *argList[] = {
            "./processR",
            i,
            "-o",
            argv[4],
            NULL
        };

        execve("./processR", argList, buffer /* environment variables in array */);

        perror("Execve returned so it's an error ");
        exit(EXIT_FAILURE);
    }



}

