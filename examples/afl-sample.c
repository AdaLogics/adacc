#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int foo(char *arr, int t1) {
    int i = 0;

    if (arr[i++] == 'A') return 0;
    if (arr[i++] == 'B') return 1;
    if (arr[i++] == 'C') return 2;
    if (arr[i++] == 'C') return 2;
    if (arr[i++] == 'D') return 3;
    if (arr[i++] == 'E') return 4;
    if (arr[i++] == 'F') return 5;
    if (arr[i++] == 'G') return 6;
    if (*(int*)(arr) != 0xdeadbeef) { 
        return 0;
    }
    return (int)20/t1;
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

    // call into target
    int retval = foo(string, argc-2);

    free(string);
    return retval;
}
#else
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    target(data, size);
    return 0;
}
#endif
