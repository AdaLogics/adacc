#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int foo(char *arr, int t1) {
    int i = 0;

    printf("Readgin\n");
    if (arr[i++] != 'A') return 0;
    printf("1\n");
    if (arr[i++] != 'B') return 1;
    printf("2\n");
    if (arr[i++] != 'C') return 2;
    printf("3\n");
    if (arr[i++] != 'D') return 3;
    printf("4\n");
    if (arr[i++] != 'E') return 4;
    printf("5\n");
    if (arr[i++] != 'F') return 5;
    printf("6\n");
    if (arr[i++] != 'G') return 6;
    printf("7\n");
    if (arr[i++] != 'H') return 7;
    printf("8\n");
    if (arr[i++] != 'I') return 7;
    if (arr[i++] != 'J') return 7;
    printf("9\n");
    if (arr[i++] != 'K') return 7;
    if (arr[i++] != 'L') return 7;
    printf("10\n");
    if (arr[i++] != 'M') return 7;
    if (arr[i++] != 'N') return 7;
    printf("11\n");
    if (arr[i++] != 'O') return 7;

    if (*(int*)(arr+i) != 0xdeadbeef) { 
        printf("This is not deadbeef\n");
        return 0;
    }

    // Can we trigger this code? 
    //return (int)(20 / t1);
    printf("Goaaaaaal !\n");
    return 2;
}

int main(int argc, char* argv[]) {
    // open file
    FILE *f = fopen(argv[1], "rb");

    // get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);

    // read file contents
    fseek(f, 0, SEEK_SET);
    char *string = (char*)malloc(fsize + 1);
    fread(string, 1, fsize, f);
    fclose(f);

    // pass string to foo
    int retval = foo(string, argc-2);

    free(string);
    return retval;
}
