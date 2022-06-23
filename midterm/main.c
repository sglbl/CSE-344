#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include "clientX.h"
#include "serverY.h"

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
    char *dataFilePath, *serverFifoPath;

    while ((option = getopt(argc, argv, "s:o:")) != -1) {
        switch (option) {
            case 'o':
                dataFilePath = optarg;
                break;
            case 's':
                serverFifoPath = optarg;
                break;
            default:
                write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
                exit(EXIT_FAILURE);
        }
    }

    handleDataFile(dataFilePath, serverFifoPath);

    // Unblocking signals
    if( sigprocmask(SIG_UNBLOCK, &oldSetMask, NULL) == -1 ){
        perror("sigprocmask error ");
        write(STDERR_FILENO, "Usage: ./client -s pathToServerFifo -o pathToDataFile\n", 55);
        exit(EXIT_FAILURE);
    }

    return 0;
}