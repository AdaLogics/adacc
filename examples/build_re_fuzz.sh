git clone https://github.com/kokke/tiny-regex-c
cd tiny-regex-c
SYMCC_REGULAR_LIBCXX=1 symcc -fsanitize-coverage=inline-8bit-counters -c -I./ ../fuzz_re.c -o fuzz_re.o
SYMCC_REGULAR_LIBCXX=1 symcc -fsanitize-coverage=inline-8bit-counters -c -I./ ./re.c -o re.o
cd ../
SYMCC_REGULAR_LIBCXX=1 symcc libfuzz-harness-proxy.c -fsanitize-coverage=inline-8bit-counters ./tiny-regex-c/fuzz_re.o ./tiny-regex-c/re.o ../models/klee-libc/minilibc.a -o symcc-tiny-regex-re-fuzz


