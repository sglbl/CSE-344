#include <stdio.h>

int main(int argc, char const *argv[])
{
    printf("%d\n", sg_strlen("sglbl"));
    printf("- %s\n", sg_strcat("sglbl", "mglbl"));
    printf("- %s\n", sg_strncat("sglbl", "mglbl", 3));
    printf("- %d\n", sg_boolStrcmp("sglbl", "mglbl"));
    printf("%d\n", sg_strncmp("sglbl", "sglbl", 3));
    printf("- %d\n", sg_boolStrncmp("sglbl", "sglbl", 6));

    char source[] = "Techie Delight";
    char destination[25];
    sg_strncpy(destination, source, 5);
    // printf("%s\n", destination);


    char** oper = (char**)malloc(2 * sizeof(char*));
    char* string = "/str1/str2/;/str3/str4/";

    int delimIndex = 0;
    char *token = sg_strtok(string, ';', &delimIndex);
    oper[0] = token;
    for(int i = 0; token != NULL; i++){
        string = string + delimIndex + 1;
        token = sg_strtok(string, ';', &delimIndex);
        oper[i+1] = token;
    }

    for(int i=0; i<2; i++)
        printf("oper[%d] is %s\n",i ,oper[i]);


    return 0;
}
    