#include <stdio.h>
#include <signal.h>

int didSignalCame = 0;

static void pSigHandler(int signo){
    switch (signo) {
        case SIGTSTP:
            printf("TSTP");
            fflush(stdout);
            break;
        case SIGINT:
            printf("INT\n");
            didSignalCame = 1;
            fflush(stdout);
            break;
    }
}

int main(void) {
    struct sigaction psa;
    psa.sa_handler = pSigHandler;
    sigaction(SIGINT, &psa, NULL);
    printf("Starting loop\n");
    for(;;) {
        if(didSignalCame == 1)  
            break;
    }
    return 0;
}