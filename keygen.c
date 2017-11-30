#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main (int argc, char** argv)
{
    int i;
    int l = 0;
    const char* j = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    char* k;

    srand(time(NULL));

    // Check argument count.
    if (argc < 2) exit(0);

    // String to integer.
    for (i = 0; argv[1][i] != '\0'; ++i)
        l = l*10 + argv[1][i] - '0';

    k = malloc(l * sizeof(char) + 1);


    // Generate code 
    for (i = 0; i < l; i++)
        k[i] = j[rand() % 27];

    k[l] = 0;

    printf("%s\n", k);

    free(k);
    k = 0;
}
