cat ./corpus_counters.stats | wc -l
sha256sum ./corpus_counters.stats
echo "Number of out files"
ls -la ./wdir-23/out | wc -l
WD=./tmps-16.txt
echo "This is SymCC running with the QSYM backend"
cat ${WD} | grep "This is SymCC running with the QSYM backend" | wc -l 
echo  "Found counter not in old map"
cat ${WD} | grep "Found counter not in old map" | wc -l
echo "not save"
cat ${WD} | grep "not save" | wc -l
echo "Should save"
cat ${WD} | grep "Should save" | wc -l
