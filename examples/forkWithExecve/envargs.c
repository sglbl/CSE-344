#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

extern char **environ;

/* 
    cc t_execve.c -o t_execve  && cc envargs.c -o envargs
    ./t_execve ./envargs
*/

int main(int argc, char *argv[])
{
    int j;
    char **ep;
    for (j = 0; j < argc; j++)
        printf("argv[%d] = %s\n", j, argv[j]);
    for (ep = environ; *ep != NULL; ep++)
        printf("environ: %s\n", *ep);
    exit(EXIT_SUCCESS);
}