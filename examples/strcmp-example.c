#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

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
  /*
  char *tmp_p = start;
  while (tmp_p < end) {
    char c = *tmp_p;
    printf("Counter value: %d\n", (int)c);
    tmp_p++;
  }*/
  __s2anitizer_cov_8bit_counters_init(start, end);
}


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
