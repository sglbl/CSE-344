#include <stdio.h>
#include <stdlib.h> //for memory allocation
#include <string.h>

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
    int i;
    for(i=0; *source != '\0' && i < sg_strlen(source); i++)
        dest[i] = source[i];
    dest[i] = '\0';

}

char* sg_strtok(char* str, char delimiter, int *delimIndex){
    if(str != NULL){
        for(int i=0; i < sg_strlen(str); i++){
            if(str[i] == delimiter){
                char* token = (char*)malloc(i*sizeof(char));
                for(int j=0; j< i; j++)
                    token[j] = str[j];
                // delimIndex pointer will hold the i which is index of delimiter. So str index will be changed every time moving forward str+*delimIndex
                *delimIndex = i;
                return token;
            }
        }
        if(*delimIndex == sg_strlen(str)){
            *delimIndex = -1;
            return str;
        }
        return NULL;
    }
    else{  // str = NULL
        printf("Error while parsing. Value is null\n");
        return NULL;
    }
}

// char* sg_strtok(char** str, char delimiter){
//     if(str != NULL){
//         for(int i=0; i < sg_strlen(*str); i++){
//             if(*str[i] == delimiter){
//                 char* token = (char*)malloc(i*sizeof(char));
//                 char* temp  = (char*)malloc(i*sizeof(char));
//                 for(int j=0; j< sg_strlen(str); j++)
//                     temp[j] = str[i+1];

//                 // sg_strcpy(temp, *str + i + 1);
//                 printf("temp is %s\n", temp);
//                 *str = temp;
//                 printf("token is %s", token);
//                 // printf("str is %s\n", str);
//                 return token;
//             }
//         }
//     }
//     else{  // str = NULL
//         printf("Error while parsing. Value is null\n");
//     }
//     return NULL;
// }