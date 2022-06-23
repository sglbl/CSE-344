#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sg_process_p.h"
#include "sg_process_r.h"

int main(int argc, char *argv[]){
    // Initialization of blocking signals
    sigset_t blockSetMask, oldSetMask;  // Set of signals to block
    sigemptyset(&blockSetMask); // Initializing the set of signals to block.
    sigemptyset(&oldSetMask);
    sigfillset(&blockSetMask);  // Filling set will all signals. [SIGKILL and SIGSTOP cannot be blocked]
    sigdelset(&blockSetMask, SIGINT); // Remove SIGINT from the set of signals to block [because we want to use the signal handler function for SIGINT]
    // Blocking signals
    if( sigprocmask(SIG_BLOCK, &blockSetMask, &oldSetMask) == -1 ){
        perror("sigprocmask error ");
        exit(EXIT_FAILURE);
    }

    char *filePath = argv[2]; // Input file path
    struct stat statOfFile;   // Adress of statOfFile will be sent to stat() function in order to get size information of file.
    int fileDesc;             // Directory stream file descriptor for file reading and writing

    if( stat(filePath, &statOfFile) < 0 || argc != 5 ){
        perror("Error while opening file. ");
        printUsageAndExit();
    }

    // Opening file in read mode
    if( (fileDesc = open(filePath, O_RDONLY, S_IWGRP)) == -1 ){
        perror("Error while opening file to read. ");
        printUsageAndExit();
    }

    // Reading file and inside this method creating child processes
    reader(fileDesc, argv, statOfFile.st_size);

    // Reading is completed. Closing the file.
    if( close(fileDesc) == -1 ){   
        perror("Error while closing the file. ");
        exit(EXIT_FAILURE);
    }

    // Unblocking signals
    if( sigprocmask(SIG_UNBLOCK, &oldSetMask, NULL) == -1 ){
        perror("sigprocmask error ");
        exit(EXIT_FAILURE);
    }

    return 0;
}