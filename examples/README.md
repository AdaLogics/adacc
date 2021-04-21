Ensure you have the correct environment variables set.
```
mkdir inp && echo "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" >> inp/seed1

SYMCC_REGULAR_LIBCXX=1 sym++ ./int_check.c -o int_check
../util/min-concolic-exec.sh -i ./inp -o ./outs ./int_check @@ 
