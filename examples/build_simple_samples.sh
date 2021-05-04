
#SYMCC_REGULAR_LIBCXX=1 symcc -fsanitize-coverage=trace-pc-guard ./simple-example.c -o simple-example
#SYMCC_REGULAR_LIBCXX=1 symcc -fsanitize-coverage=inline-8bit-counters ./simple-example.c -o simple-example
#SYMCC_REGULAR_LIBCXX=1 sym++ ./simple-example.c -o simple-example
#SYMCC_REGULAR_LIBCXX=1 symcc -fsanitize-coverage=inline-8bit-counters ./strcmp-example.c ../models/klee-libc/minilibc.a -o strcmp-example
#SYMCC_REGULAR_LIBCXX=1 sym++ ./symcc-issue-23.c ../models/klee-libc/minilibc.a -o symcc-issue-23-example


SYMCC_REGULAR_LIBCXX=1 symcc ./simple-example.c -o simple-example


#sym++ ./hello-world.cpp -o hello-world
