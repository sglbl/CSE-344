#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sg_replacer.h"

int main(int argc, char *argv[]){

    struct stat statOfFile;  //Adress of statOfFile will be sent to stat() function in order to get size information of file.
    int readedBytes;            // Number of readed bytes
    int fileDesc;        // Directory stream file descriptors for reading and writing
    struct flock lock;
    char *filePath = argv[2];   // Path of file that is given on the last command line argument

    if(stat(filePath, &statOfFile) < 0){
        perror("Error while opening the file.\n");
        exit(EXIT_FAILURE);
    }

    char *buffer = (char*)calloc( statOfFile.st_size , sizeof(char));   // Buffer that data will be stored
    if(argc != 3)
        printErrorAndExit();    
    
    // OPENING FILE ON READ ONLY MODE
    if( (fileDesc = open(filePath, O_RDWR , S_IWGRP)) == -1 ){
        perror("Error while opening the file to read.\n");
        exit(2);
    }

    // LOCKING
    memset(&lock, 0, sizeof(lock) );  // Init the lock system
    lock.l_type = F_WRLCK; // F_WRLCK: field of the structure for a write lock.
    if ( fcntl(fileDesc, F_SETLKW, &lock) == -1) { // Putting write lock on the file. F_SETLKW: If a signal is caught while waiting, then the call is interrupted and (after signal handler returned) returns immediately
        perror("Error while locking with fcntl(F_SETLK)");
        exit(EXIT_FAILURE);
    }

    // READING FILE AND SAVING CONTENT TO BUFFER
    while( (readedBytes = read(fileDesc, buffer, statOfFile.st_size)) == -1 && errno == EINTR ){ /* Intentionanlly Empty loop to deal interruptions by signal */}
    if(readedBytes <= 0){
        perror("File is empty. Goodbye\n");
        exit(3);
    }

    // PARSING OPERATIONS FROM COMMAND LINE ARGUMENT TO AN ARRAY
    int size = 0; //Will be used with call by reference
    //char* command = "/^Window[sz]*/Linux/i;/close[dD]$/open/";
    // char** operations = argDivider( command, &size );
    char** operations = argDivider( argv[1], &size );

    // REPLACING ON THE BUFFER VARIABLE
    replacer(buffer, operations, size);
    // IF OPERATIONS FOUND ON FILE, BUFFER IS CHANGED

    write(1 /* fd for stdout */, "New Buffer to be written is '", 30); 
    write(1 /* fd for stdout */, buffer, strlen(buffer) );
    write(1 /* fd for stdout */, "'\n", 3);

    // SETTING CURSOR TO THE BEGINNING OF FILE WITH LSEEK
    lseek(fileDesc, 0, SEEK_SET); 
    // TRUNCATING OLD INFO IN FILE DESCRIPTOR'S BUFFER SO WE CAN OVERWRITE AGAIN IN THE SAME FILE.
    ftruncate(fileDesc, strlen(buffer) ); 

    // WRITING THE NEW BUFFER INTO THE SAME INPUT FILE
    while( write(fileDesc, buffer, strlen(buffer) ) == -1 && errno == EINTR ){/* Intentionanlly Empty loop to deal interruptions by signal */}
    write(1 /* fd for stdout */, "Succesfully writed to the file\n", 32);

    // UNLOCKING 
    lock.l_type = F_UNLCK;
    if ( fcntl(fileDesc, F_SETLKW, &lock) == -1) {
        perror("Error while unlocking with fcntl(F_SETLK)");
        exit(EXIT_FAILURE);
    }

    // WRITING IS COMPLETED. CLOSING THE FILE.
    if( close(fileDesc) == -1 ){   
        perror("Error while closing the file.");
        exit(6);
    }

    return 0;
}