#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include "sg_process_p.h"

void printErrorAndExit(){
    perror( "ERROR FOUND ON ARGUMENTS; PLEASE ENTER A VALID INPUT! INSTRUCTIONS:\n"
    "./processP -i inputFilePath -o outputFilePath\n");
    write(1 /* File descriptor for stdout */, "Goodbye!\n", 10);
    exit(EXIT_FAILURE);
}

void reader(int fileDescriptor, char *argv[]){
    int readedByte;
    int i = 1; //Name of the children will be R_i

    cleanTheOutputFile(argv);

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
        //Creating child processes with i value with fork() and execve()
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

        char *argList[] = {
            "./processR",
            i,
            "-o",
            argv[4], // name of the output file comes from command line argument
            NULL
        };

        execve("./processR", argList, buffer /* environment variables in array */);

        perror("Execve returned so it's an error ");
        exit(EXIT_FAILURE);
    }

}

void cleanTheOutputFile(char *argv[]){
    int fileDesc;
    if( (fileDesc = open(argv[4], O_WRONLY | O_TRUNC, S_IWGRP)) == -1 ){ // argv[4] is the name of the output file
        perror("Error while opening file to write.\n");
        exit(EXIT_FAILURE);
    }
    // Now our output file is empty.
    if( close(fileDesc) == -1 ){   
        perror("Error while closing the file.");
        exit(EXIT_FAILURE);
    }

}
