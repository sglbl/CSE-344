#include <stdio.h>
#include <stdlib.h> //for memory allocation
/*
	@author Suleyman Golbol
	@number 1801042656
*/

int sg_strlen(char* string){
    int counter = 0;
    while( string[counter] != '\0' )
        counter++;
    return counter;
}

int sg_strcmp(char* str1, char* str2){
    if( sg_strlen(str1) != sg_strlen(str2) )
        return -1;
        
    for(int i=0; i < sg_strlen(str1); i++)
        if(str1[i] != str2[i])
            return -1;
    return 0;
}

int sg_strncmp(char* str1, char* str2, int n){
    for(int i=0; i < n; i++)
        if(str1[i] != str2[i]){
            return -1;
        }
    return 0;
}

void sg_strncpy(char* dest, char* source, int n){
    if(dest == NULL || source == NULL){
        fprintf(stderr,  "The destination or source string is null\n");
        exit(EXIT_FAILURE);
    }
    int i;
    for(i=0; *source != '\0' && i < n; i++){
        dest[i] = source[i];
    }
    dest[i] = '\0';

}

void sg_strcpy(char* dest, char* source){
    if(dest == NULL || source == NULL){
        fprintf(stderr,  "The destination or source string is null\n");
        exit(EXIT_FAILURE);
    }
    printf("zxx\n");
    int i;
    for(i=0; *source != '\0' && i < sg_strlen(source); i++)
        dest[i] = source[i];
    dest[i] = '\0';

}

char* sg_strcat(char* str1, char* str2){
    int size = 1 + sg_strlen(str1) + sg_strlen(str2);  // plus 1 because of '\0' character
    char* string = (char*)calloc(size, sizeof(char));

    for(int i=0; i < size; i++){
        if( i < sg_strlen(str1) )
            string[i] = str1[i];
        else if(i == size - 1)
            string[i] = '\0';
        else
            string[i] = str2[i - sg_strlen(str1)];
    }
    return string;
}

char* sg_strncat(char* str1, char* str2, int n){
    int size = 1 + sg_strlen(str1) + n;
    int prevSize = sg_strlen(str1);
    if(n > sg_strlen(str2)) {
        fprintf(stderr,  "Size of str2 cannot be bigger than n\n");
        exit(0);
    }

    char* string = (char*)calloc(size, sizeof(char));

    for(int i=0; i < sg_strlen(str1); i++)
        string[i] = str1[i];
    for(int i=0; i < n ; i++)
        string[prevSize + i] = str2[i];
    string[size-1] = '\0';

    return string;
}