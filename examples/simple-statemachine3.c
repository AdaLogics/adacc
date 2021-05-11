#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void ps(int state) {
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


void target(char *string, size_t size) {
    // pass string to foo

    if (size > 10) size = 10;
    int state = 0;
    for (int i = 0; i < size; i++) {

        char c = string[i];
        switch (c) {
            case 'A':
                printf("Hello 1\n");
                state = 3;
                break;
            case 'B':
                printf("Hello 1\n");
                if (state == 3) {
                    state = 4;
                    printf("H1\n");
                }
                else { state = 2; }
                break;
            case 'C':
                printf("Hello 1\n");
                if (state == 4) {
                    state = 1;
                }
                break;
            case 'D':
                printf("Hello 1\n");
                if (state == 1) {

                    state = 6;
                }
                break;
 
 		}
        ps(state);
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
