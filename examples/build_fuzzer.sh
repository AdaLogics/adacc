git clone https://github.com/nodejs/http-parser
cd http-parser
SYMCC_REGULAR_LIBCXX=1 symcc -c -I./ ./fuzzers/fuzz_url.c -o fuzz_url.o 
SYMCC_REGULAR_LIBCXX=1 symcc -c -I./ ./http_parser.c -o http_parser.o 
cd ../
SYMCC_REGULAR_LIBCXX=1 symcc libfuzz-harness-proxy.c ./http-parser/fuzz_url.o ./http-parser/http_parser.o ../models/klee-libc/minilibc.a -o symcc-http-parse-fuzz-url 


cd http-parser
SYMCC_REGULAR_LIBCXX=1 symcc  -c -I./ ./fuzzers/fuzz_parser.c -o fuzz_parser.o 
SYMCC_REGULAR_LIBCXX=1 symcc  -c -I./ ./http_parser.c -o http_parser.o 
cd ../
SYMCC_REGULAR_LIBCXX=1 symcc libfuzz-harness-proxy.c  ./http-parser/fuzz_parser.o ./http-parser/http_parser.o ../models/klee-libc/minilibc.a -o symcc-http-parse-fuzz-parser
