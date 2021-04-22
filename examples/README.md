Ensure you have `sym++` in your PATH variable.

```
mkdir inp && echo "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" >> inp/seed1

SYMCC_REGULAR_LIBCXX=1 sym++ ./simple-example.c -o simple-example
../util/min-concolic-exec.sh -i ./inp/ -o ./outs ./simple-example @@
...
...
...
[STAT] SMT: { "solving_time": 9661, "total_time": 15750 }
[STAT] SMT: { "solving_time": 10254 }
Pushing path constraint
Current path: 0000000000000000
Next path: 0000000000000001
Should not save this path
[STAT] SMT: { "solving_time": 10254, "total_time": 16667 }
[STAT] SMT: { "solving_time": 10672 }
Goaaaaaal !
Copying /tmp/tmp.xAjLyvdnSX/symcc_out
No more inputs, breaking
Completed execution
Total inputs found: 17
Total paths found: 32
```

We can now observe the outputs of symcc in the `outs` folder. The statistics tell us the number of paths found as well as the number of unique inputs. 

We can now run our binary (compiled without symcc instrumentation) on the various inputs and verify the results:


```
clang ./simple-example.c -o native-simple-example
$ find ./outs -type f -exec ./native-simple-example {} \;
Fail
Fail
Fail
Fail
Fail
Fail
Fail
Fail
Goaaaaaal!
Fail
Fail
Fail
Fail
Fail
Fail
Fail
```


## with models
First build the models in `/models/klee-libc` by 
```
cd /models/klee-libc
./build.sh
```

This will produce `minilibc.a`

Now you can compile c applications that use a set of functions compiled by symcc:

```
SYMCC_REGULAR_LIBCXX=1 sym++ strcmp-example.c ../models/klee-libc/minilibc.a -o strcmp-example
../util/min-concolic-exec.sh -i ./inp -o ./outs ./strcmp-example @@
...
...
```
