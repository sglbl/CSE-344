#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include "sg_process.h"

#define TRUE 1
#define FALSE 0

#define CHILD_SIZE 10
#define COORD_DIMENSIONS 3

void printErrorAndExit(){
    perror( "ERROR FOUND ON ARGUMENTS; PLEASE ENTER A VALID INPUT! INSTRUCTIONS:\n"
    "./processP -i inputFilePath -o outputFilePath\n");
    write(1 /* File descriptor for stdout */, "Goodbye!\n", 10);
    exit(EXIT_FAILURE);
}

int spawn(char** argv, char** buffer, int i){
    pid_t pidCheckForChild = fork(); // Duplicating this process.
    if( pidCheckForChild != 0){
         // pidCheckForChild is not 0, so it's the parent process.
        return pidCheckForChild;
    }
    else{ 
        // pidCheckForChild is 0 so we are in child process.
        printf("This is the child.\n");
        // execlp("/bin/ls","ls",(char *) 0);
        
        // char *arg_list[] = {
        //     "ls",
        //     "-l",
        //     "/",
        //     NULL
        // };
        // execve("/bin/ls", arg_list, buffer);
        
        execve("/bin/ls", argv, buffer /* environment variables in array */);

        // execve("files/newPath.txt", arg_list, buffer /* environment variables in array */);
        perror("Execve returned so it's an error ");
        exit(EXIT_FAILURE);
    }

    


}

void reader(int fileDescriptor, char *argv[]){
    int readedByte;
    int i = 0;

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

        spawn(argv, buffer, i);
        i++;
    }    

    
}


char** argDivider(char* arg, int *counter){
    // int i;
    // if( strncmp(arg, "/", 1) != 0){  // If first letter of argument is not '/' exit.
    //     printErrorAndExit();
    // }

    // //Finding number of operations
    // int numberOfOperations = 1;
    // for(i = 0; i < strlen(arg); i++){
    //     if(arg[i] == ';') 
    //         numberOfOperations++;  
    // }

    // char** operations = (char**)calloc(numberOfOperations, sizeof(char*));
    // char *token = strtok(arg, ";");
    // for(i = 0; token != NULL && i < numberOfOperations; i++){
    //     operations[i] = token;
    //     token = strtok(NULL, ";");
    // }
    // *counter = i;
    
    // return operations;
    return NULL;
}
