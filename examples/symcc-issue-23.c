#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
int main(int argc, char *argv[]) {


    FILE *f = fopen(argv[1], "rb");

    // get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);

    // read file contents
    fseek(f, 0, SEEK_SET);
    char *buf = (char*)malloc(fsize + 1);
    fread(buf, 1, fsize, f);
    buf[fsize] = '\0';
    fclose(f);


//  buf[i] = 0;
  if (buf[0] != 'A') return 0;
  if (buf[1] != 'B') return 0;
  if (buf[2] != 'C') return 0;
  if (buf[3] != 'D') return 0;
  if (memcmp(buf + 4, "1234", 4) || memcmp(buf + 8, "EFGH", 4)) return 0;
  if (strncmp(buf + 12, "IJKL", 4) == 0 && strcmp(buf + 16, "DEADBEEF") == 0)
    abort();
  return 0;

}
