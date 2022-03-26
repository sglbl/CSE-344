#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include "sg_process_r.h"

extern char **environ;  // Environment variables that will be passed to child process and will come from "sg_process_p.c"

int main(int argc, char *argv[]){
    
    int i = argv[1][0];
    char *filePath = argv[3]; // Output file path
    int fileDesc;             // Directory stream file descriptor for file writing
    struct flock lock;        // Lock structure of the file.

    // Opening file in read/write mode
    if( (fileDesc = open(filePath, O_WRONLY | O_APPEND, S_IWGRP)) == -1 ){
        perror("Error while opening file to write.\n");
        exit(EXIT_FAILURE);
    }

    // Locking
    memset(&lock, 0, sizeof(lock)); //Initing structure of lock to 0.
    lock.l_type = F_WRLCK;  // F_WRLCK: Field of the structure of type flock for write lock.
    if( fcntl(fileDesc, F_SETLKW, &lock) == -1 ){ // putting write lock on file. F_SETLKW: If a signalis caught while waiting, then the call is interrupted and (after signal handler returned) returns immediately.
        perror("Error while locking fcntl(F_SETLK mode).\n");
        exit(EXIT_FAILURE);
    }

    for(int j = 0; j < CHILD_SIZE; j++){
        while( write(fileDesc, "(", 1 ) == -1 && errno == EINTR )   {/* Intentionanlly Empty loop to deal interruptions by signal */}
        for(int k = 0; k < COORD_DIMENSIONS; k++){
            while( write(fileDesc, &environ[j][k], sizeof(char) ) == -1 && errno == EINTR ){}
            while( write(fileDesc, ",", sizeof(char) ) == -1 && errno == EINTR ){}
        }
        while( write(fileDesc, "),", 2*sizeof(char) ) == -1 && errno == EINTR ){}
    }
    while( write(fileDesc, "\n", sizeof(char) ) == -1 && errno == EINTR ){}

    printf("Created R_%d with (%d,%d,%d),(%d,%d,%d),...,(%d,%d,%d)\n", i,
            environ[0][0], environ[0][1], environ[0][2],
            environ[1][0], environ[1][1], environ[1][2],
            environ[CHILD_SIZE-1][0], environ[CHILD_SIZE-1][1], environ[CHILD_SIZE-1][2]);

    // Unlocking
    lock.l_type = F_UNLCK;
    if ( fcntl(fileDesc, F_SETLKW, &lock) == -1) {
        perror("Error while unlocking with fcntl(F_SETLKW)");
        exit(EXIT_FAILURE);
    }

    if( close(fileDesc) == -1 ){   
        perror("Error while closing the file.");
        exit(EXIT_FAILURE);
    }

    return 0;
}