SYMCC_REGULAR_LIBCXX=1 sym++ ./simple-example.c -o simple-example
SYMCC_REGULAR_LIBCXX=1 sym++ ./strcmp-example.c ../models/klee-libc/minilibc.a -o strcmp-example
