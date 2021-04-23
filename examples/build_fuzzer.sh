git clone https://github.com/nodejs/http-parser
cd http-parser
SYMCC_REGULAR_LIBCXX=1 sym++ -c -I./ ./fuzzers/fuzz_url.c -o fuzz_url.o 
SYMCC_REGULAR_LIBCXX=1 sym++ -c -I./ ./http_parser.c -o http_parser.o 
cd ../
SYMCC_REGULAR_LIBCXX=1 sym++ libfuzz-harness-proxy.c ./http-parser/fuzz_url.o ./http-parser/http_parser.o -o symcc-http-parse-fuzz-url
