#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h> // Variadic function
#include <time.h>  // Time stamp
#include <errno.h> // spesific error message
#include <fcntl.h> // provide control over open files
#include <unistd.h> // unix standard functions
#include <sys/types.h> // Types of signals
#include <sys/stat.h> // Stat command
#include <signal.h> // Signal handling
#include <pthread.h> // Threads and mutexes
#include "../include/common.h"

char *timeStamp(){
    time_t currentTime;
    currentTime = time(NULL);
    char *timeString = asctime( localtime(&currentTime) );

    // Removing new line character from the string
    int length = strlen(timeString);
    if (length > 0 && timeString[length-1] == '\n') 
        timeString[length - 1] = '\0';
    return timeString;
}

void errorAndExit(char *errorMessage){
    // perror(errorMessage);
    dprintf(STDERR_FILENO, "(%s) %s: %s\n", timeStamp(), errorMessage, strerror(errno));
    exit(EXIT_FAILURE);
}