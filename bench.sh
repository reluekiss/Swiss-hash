#!/bin/sh
set -x

rm -rf .temp
mkdir -p .temp

for i in boost boost8 boostr boostr8 ska ska8 skar skar8; do
    g++ -O5 -march=native profiling/"$i".cc -o .temp/"$i"
    ./.temp/"$i" > .temp/"$i".csv
done

for i in swiss swiss8 swissr swissr8; do
    gcc -O5 -march=native -D__AVX2__ profiling/"$i".c xxhash3.c hash.c -o .temp/"$i"
    ./.temp/"$i" > .temp/"$i".csv
done

python3 plot.py
