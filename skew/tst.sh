#!/bin/bash
R=$2
if [ X$R == X ]
then
R=/export/scratch1/boncz/msc-code-copy-tim/numaopt/x100_old/tests/tpch/SF-0.01/$1 
fi

for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
do
echo ===== param $i
awk -F '|' '{ if (c[$'$i']++ == 0) v[k++]=$'$i'} END { for(i=0;i<k;i++) print v[i] "|" c[v[i]]}' $1 | awk -F '|' '{ if (c[$2]++ == 0) v[k++]=$2} END { for(i=0;i<k;i++) print v[i] "|" c[v[i]]}' | sort -t '|' -nrk 1
echo == orig
awk -F '|' '{ if (c[$'$i']++ == 0) v[k++]=$'$i'} END { for(i=0;i<k;i++) print v[i] "|" c[v[i]]}' $R | awk -F '|' '{ if (c[$2]++ == 0) v[k++]=$2} END { for(i=0;i<k;i++) print v[i] "|" c[v[i]]}' | sort -t '|' -nrk 1
done
