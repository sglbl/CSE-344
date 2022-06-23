#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>


/* 
    cc t_execve.c -o t_execve  && cc envargs.c -o envargs
    ./t_execve ./envargs
*/

int main(int argc, char *argv[])
{
    char *argVec[10]; /* Larger than required */
    if (argc != 2 || strcmp(argv[1], "--help") == 0)
        printf("%s pathname\n", argv[0]);
    argVec[0] = strrchr(argv[1], '/'); /* Get basename from argv[1] */ //returns str after '/' sign.
    if (argVec[0] != NULL)
        argVec[0]++;
    else
        argVec[0] = argv[1];

    argVec[1] = "hello world";
    argVec[2] = "goodbye";
    argVec[3] = NULL; /* List must be NULL-terminated */

    char *envVec[] = {"GREET=salut", "BYE=adieu", NULL};
    execve(argv[1], argVec, envVec);
    perror("execve"); /* If we get here, something went wrong */
}
// Imagine the input from the commandline is ‘/bin/ls’