#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include "sg_process_r.h"

extern char **environ;

int main(int argc, char *argv[]){
    // for (int k = 0; k < argc; k++)
    //     printf("argv[%d] = %s\n", k, argv[k]);
    int i = argv[1][0];
    char *filePath = argv[3]; // Output file path
    //struct stat statOfFile; // Adress of statOfFile will be sent to stat() function in order to get size information of file.
    int fileDesc;             // Directory stream file descriptor for file writing

    // Opening file in read/write mode
    if( (fileDesc = open(filePath, O_WRONLY | O_APPEND, S_IWGRP)) == -1 ){
        perror("Error while opening file to write.\n");
        exit(EXIT_FAILURE);
    }

    for(int j = 0; j < 10; j++){
        // printf("environ[%d] = %s\n", j, environ[j]);
        while( write(fileDesc, "(", 1 ) == -1 && errno == EINTR )
            {/* Intentionanlly Empty loop to deal interruptions by signal */}
        for(int k = 0; k < 3; k++){
            while( write(fileDesc, &environ[j][k], sizeof(char) ) == -1 && errno == EINTR ){}
            while( write(fileDesc, ",", sizeof(char) ) == -1 && errno == EINTR ){}
        }
        while( write(fileDesc, "),", 2*sizeof(char) ) == -1 && errno == EINTR ){}
    }
    while( write(fileDesc, "\n", sizeof(char) ) == -1 && errno == EINTR ){}

    printf("Created R_%d with (%d,%d,%d),(%d,%d,%d),...,(%d,%d,%d)\n", i,
            environ[0][0], environ[0][1], environ[0][2],
            environ[1][0], environ[1][1], environ[1][2],
            environ[9][0], environ[9][1], environ[9][2]);

    if( close(fileDesc) == -1 ){   
        perror("Error while closing the file.");
        exit(EXIT_FAILURE);
    }

    return 0;
}