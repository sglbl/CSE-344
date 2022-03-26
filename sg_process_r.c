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
    
    int i = argv[1][0];       // Special number of this child.
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
    if( fcntl(fileDesc, F_SETLKW, &lock) == -1 ){ // putting write lock on file. 
        // F_SETLKW: If a signal is caught while waiting, then the call is interrupted and (after signal handler returned) returns immediately.
        perror("Error while locking fcntl(F_SETLK mode).\n");
        exit(EXIT_FAILURE);
    }

    // Writing to file
    for(int j = 0; j < CHILD_SIZE; j++){
        while( write(fileDesc, "(", 1 ) == -1 && errno == EINTR )   {/* Intentionanlly Empty loop to deal interruptions by signal */}
        for(int k = 0; k < COORD_DIMENSIONS; k++){
            while( write(fileDesc, &environ[j][k], sizeof(char) ) == -1 && errno == EINTR ){}
        }
        while( write(fileDesc, ")", sizeof(char) ) == -1 && errno == EINTR ){}
    }
    while( write(fileDesc, "\n", sizeof(char) ) == -1 && errno == EINTR ){}

    // Printing child info
    printChildInfo(i);

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

/* Printing child information */
void printChildInfo(int childNumber){
    // Because of printf and snprintf are not signal safe, I used write(). 
    // For formatting from int to string I used itoaForAscii(int) function.
    
    write(STDOUT_FILENO, "Created R_", 11);
    write(STDOUT_FILENO, itoaForAscii(childNumber), sizeof( itoaForAscii(childNumber) ) );
    write(STDOUT_FILENO, " with (", 8);
    write(STDOUT_FILENO, itoaForAscii( environ[0][0] ), sizeof( itoaForAscii(environ[0][0]) ) );
    write(STDOUT_FILENO, ",", 1);
    write(STDOUT_FILENO, itoaForAscii( environ[0][1] ), sizeof( itoaForAscii(environ[0][1]) ) );
    write(STDOUT_FILENO, ",", 1);
    write(STDOUT_FILENO, itoaForAscii( environ[0][2] ), sizeof( itoaForAscii(environ[0][2]) ) );
    write(STDOUT_FILENO, "), (", 4);
    write(STDOUT_FILENO, itoaForAscii( environ[1][0] ), sizeof( itoaForAscii(environ[1][0]) ) );
    write(STDOUT_FILENO, ",", 1);
    write(STDOUT_FILENO, itoaForAscii( environ[1][1] ), sizeof( itoaForAscii(environ[1][1]) ) );
    write(STDOUT_FILENO, ",", 1);
    write(STDOUT_FILENO, itoaForAscii( environ[1][2] ), sizeof( itoaForAscii(environ[1][2]) ) );
    write(STDOUT_FILENO, "), ..., (", 10);
    write(STDOUT_FILENO, itoaForAscii( environ[CHILD_SIZE-1][0] ), sizeof( itoaForAscii( environ[CHILD_SIZE-1][0] ) ) );
    write(STDOUT_FILENO, ",", 1);
    write(STDOUT_FILENO, itoaForAscii( environ[CHILD_SIZE-1][1] ), sizeof( itoaForAscii( environ[CHILD_SIZE-1][1] ) ) );
    write(STDOUT_FILENO, ",", 1);
    write(STDOUT_FILENO, itoaForAscii( environ[CHILD_SIZE-1][2] ), sizeof( itoaForAscii( environ[CHILD_SIZE-1][2] ) ) );
    write(STDOUT_FILENO, ")\n", 2);

}

char* itoaForAscii(int number){
    if(number == 0){
        return "";
    }
    int digitCounter = 0;
    int temp = number;
    while(temp != 0){
        temp /= 10;
        digitCounter++;
    }
    
    char* string = calloc(sizeof(char), (digitCounter+1));
    for(int i = 0; i < digitCounter; i++){
        char temp = (number % 10) + '0';
        string[digitCounter-i-1] = temp;
        number /= 10;
    }
    string[digitCounter] = '\0';
    
    return string;
}