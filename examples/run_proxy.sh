rm ./corpus_counters.stats
touch ./corpus_counters.stats

target=$@

# Find the next work directory
val=$(find ./ -name "wdir-*" | wc -l)
val2=$(($val+1))
rm -rf wdir-${val2} 
mkdir wdir-${val2}
../util/min-concolic-exec.sh -i ./inp -a ./wdir-${val2} ${target} @@
