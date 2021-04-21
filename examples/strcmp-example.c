#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


int foo(char *arr, int t1) {
    if (strcmp(arr, "GOAL!") == 0) {
        return 99;
    }
    return 0;
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
    if (retval == 99) {
        printf("Goaaaaaal!\n");
    } else {
        printf("Fail\n");
    }

    free(string);
    return retval;
}
