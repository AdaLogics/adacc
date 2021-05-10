

FUZZ_FLAGS="-fsanitize=fuzzer -fprofile-instr-generate -fcoverage-mapping "
#clang-10 -DNOCOV $FUZZ_FLAGS ./simple-example.c -o fuzz_binary
#clang-10 -DNOCOV $FUZZ_FLAGS ./fuzz_re.c ./tiny-regex-c/re.c -I./tiny-regex-c -o fuzz_binary
#clang-10 -DNOCOV $FUZZ_FLAGS ./http-parser/fuzzers/fuzz_url.c ./http-parser/http_parser.c -I./http-parser -o fuzz_binary
#clang-10 -DNOCOV $FUZZ_FLAGS ./http-parser/fuzzers/fuzz_parser.c ./http-parser/http_parser.c -I./http-parser -o fuzz_binary
#clang-10 -DNOCOV $FUZZ_FLAGS ./simple-statemachine.c -o fuzz_binary
clang-10 -DNOCOV $FUZZ_FLAGS ./simple-statemachine4.c -o fuzz_binary
#clang-10 -DNOCOV $FUZZ_FLAGS ./simple-statemachine2.c -o fuzz_binary
LLVM_PROFILE_FILE=cov.profraw ./fuzz_binary -dump_coverage=1 -runs=0 -close_fd_mask=3 ./wdir-28/out/
llvm-profdata-10 merge -sparse cov.profraw -o merged_cov.profdata
llvm-cov-10 show -instr-profile=merged_cov.profdata -format=html -output-dir ./coverage_report ./fuzz_binary
