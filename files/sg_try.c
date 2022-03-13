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
    return 0;
}
    