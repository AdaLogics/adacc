#include <stdlib.h>
#include <stdint.h>
#include "re.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    char *new_m = malloc(size+1);
    for (int i=0; i < size; i++) {
       new_m[i] = data[i];
    }
    new_m[size] = '\0';

    re_compile(new_m);
    //
    /*
    switch (new_m[0]) {
        case 'B' : printf("hey1\n"); break;
        case 'C' : printf("hey1\n"); break;
        case 'D' : printf("hey1\n"); break;
        case 'E' : printf("hey1\n"); break;
        case 'F' : printf("hey1\n"); break;
    }
    */
    /*
    if (new_m[0] == 'A') {
        printf("1\n");
    }
    if (new_m[0] == 'B') {
        printf("2\n");
    }
    if (new_m[0] == 'C') {
        printf("3\n");
    }
    if (new_m[0] == 'D') {
        printf("4\n");
    }
    if (new_m[0] == 'E') {
        printf("5\n");
    }
    if (new_m[0] == 'F') {
        printf("6\n");
    }
    */

    free(new_m);
    return 0;
}
