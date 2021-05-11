#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int foo(char *arr, int t1) {
    int i = 0;

    if (arr[i++] != 'A') return 0;
    if (arr[i++] != 'B') return 1;
    if (arr[i++] != 'C') return 2;
    if (arr[i++] != 'D') return 3;
    if (arr[i++] != 'E') return 4;
    if (arr[i++] != 'F') return 5;
    if (arr[i++] != 'G') return 6;
    if (arr[i++] != 'H') return 7;
    if (arr[i++] != 'I') return 7;
    if (arr[i++] != 'J') return 7;
    if (arr[i++] != 'K') return 7;
    if (arr[i++] != 'L') return 7;
    if (arr[i++] != 'M') return 7;
    if (arr[i++] != 'N') return 7;
    if (arr[i++] != 'O') return 7;

    if (*(int*)(arr+i) != 0xdeadbeef) { 
        return 0;
    }

    // Can we trigger this code? 
    return 99;
}

void target(char *string, size_t size) {
    // pass string to foo
    int retval = foo((char*)string, (int)size);
    if (retval == 99) {
        printf("Goaaaaaal!\n");
    } else {
        printf("Fail\n");
    }
    int val1 = 0;
    for (int i=0; i < 5; i++) {
        val1 += (int)string[i];
    }
    if (val1 == 123123) printf("Heyo\n");
}

#ifndef NOCOV
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

    target(string, fsize);

    free(string);
    return 1;
}
#else
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    target(data, size);
    return 0;
}
#endif
