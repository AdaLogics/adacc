#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


void target(char *string, size_t size) {
    // pass string to foo

    if (size > 10) size = 10;
    int state = 0;
    for (int i = 0; i < size; i++) {

        char c = string[i];
        if (c == 'A') {
                printf("Hello 1\n");
                state = 3;
        }
        else if(c == 'B') {
                printf("Hello 1\n");
                if (state == 3) {
                    state = 4;
                    printf("H1\n");
                }
                else { state = 2; }
        }
        else if (c == 'C') {
                printf("Hello 1\n");
                if (state == 4) {
                    state = 1;
                }
        }
        else if(c == 'D') {
                printf("Hello 1\n");
                if (state == 1) { 
                    
                    state = 6;
                }
        }
        else if(c == 'E') {
                printf("Hello 1\n");
                if (state == 6) state = 7;
        }
        else if (c == 'F') {
                printf("Hello 1\n");
                state = 3;
        }
        else if (c == 'G') {
                printf("Hello 1\n");
                state = 0;
        }

        if (state == 1) {
            printf("state 1\n");
        }
        if (state == 2) {
            printf("state 2\n");
        }
        if (state == 3) {
            printf("state 3\n");
        }
        if (state == 4) {
            printf("state 4\n");
        }
        if (state == 5) {
            printf("state 5\n");
        }
        if (state == 6) {
            printf("state 6\n");
        }
        if (state == 7) {
            printf("state 7\n");
        }
    }
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
