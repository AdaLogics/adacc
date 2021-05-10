#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
/*
#ifndef NOCOV
extern void __s2anitizer_cov_trace_pc_guard(uint32_t *guard);
extern void __s2anitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop);

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start,
                                                            uint32_t *stop) {
        printf("Hulla hop from init\n");
        __s2anitizer_cov_trace_pc_guard_init(start, stop);
        printf("dooooooooone\n");
}

void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {
   printf("Calling into s2\n");
   __s2anitizer_cov_trace_pc_guard(guard);
   printf("done\n");
}

extern void __s2anitizer_cov_8bit_counters_init(char *start, char *end);

void __sanitizer_cov_8bit_counters_init(char *start, char *end) {
  // [start,end) is the array of 8-bit counters created for the current DSO.
  // printf("Yup yey\n");
  // Capture this array in order to read/modify the counters.
  char *tmp_p = start;
  while (tmp_p < end) {
    char c = *tmp_p;
    printf("Counter value: %d\n", (int)c);
    tmp_p++;
  }
  __s2anitizer_cov_8bit_counters_init(start, end);
}
#endif
*/


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
            case 'E':
                printf("Hello 1\n");
                if (state == 6) state = 7;
                break;
            case 'F':
                printf("Hello 1\n");
                state = 3;
                break;
            case 'G':
                printf("Hello 1\n");
                state = 0;
                break;
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
