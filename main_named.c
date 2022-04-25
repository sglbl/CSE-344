#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include "sync_named.h"

int main(int argc, char *argv[]){
    // Initialization of blocking signals
    sigset_t blockSetMask, oldSetMask;  // Set of signals to block
    sigemptyset(&blockSetMask); // Initializing the set of signals to block.
    sigemptyset(&oldSetMask);   // To set old mask at the end.
    sigfillset(&blockSetMask);  // Filling set will all signals. [SIGKILL and SIGSTOP cannot be blocked]
    sigdelset(&blockSetMask, SIGINT); // Remove SIGINT from the set of signals to block [because we want to use the signal handler function for SIGINT]
    // Blocking signals
    if( sigprocmask(SIG_BLOCK, &blockSetMask, &oldSetMask) == -1 ){
        perror("sigprocmask error ");
        exit(EXIT_FAILURE);
    }

    int option;     // Argument option
    char *inputFilePath, *name;

    while ((option = getopt(argc, argv, "i:n:")) != -1) {
        switch (option) {
            case 'i':
                inputFilePath = optarg;
                break;
            case 'n':
                name = optarg;
                break;
            default:
                dprintf(STDERR_FILENO, "Usage: ./hw3named -i inputFilePath -n name\n");
                exit(EXIT_FAILURE);
        }
    }

    arrayStorer(inputFilePath, name);
    problemHandler();

    // Unblocking signals
    if( sigprocmask(SIG_UNBLOCK, &oldSetMask, NULL) == -1 ){
        perror("sigprocmask error ");
        write(STDERR_FILENO, "Usage: ./hw3named -i inputFilePath -n name\n", 44);
        exit(EXIT_FAILURE);
    }

    return 0;
}