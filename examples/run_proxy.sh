export SYMCC_PURE_CONCOLIC=1
rm ./corpus_counters.stats
touch ./corpus_counters.stats

rm ./prev_1s.txt
touch ./prev_1s.txt

rm ./prev_0s.txt
touch ./prev_0s.txt

rm ./trace_maps.txt
touch ./trace_maps.txt

target=$@

# Find the next work directory
val=$(find ./ -name "wdir-*" | wc -l)
val2=$(($val+1))
rm -rf wdir-${val2} 
mkdir wdir-${val2}
export THE_OUT_DIR="wdir-${val2}/out"

if [ -d "./inp" ]
then
    echo "input exists - using ./inp"
else 
    echo "input directory does not exist, creating ./inp"
    mkdir inp 
    echo "ABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" >> inp/seed1
fi
../util/min-concolic-exec.sh -i ./inp -a ./wdir-${val2} ${target} @@
