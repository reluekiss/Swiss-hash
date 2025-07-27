#!/bin/sh
set -x
rm -rf .temp 
mkdir -p .temp 
cp *.c *.h profiling/swiss.c .temp
cd .temp
cc -fprofile-arcs -ftest-coverage -Wall -Wextra -march=native -D__AVX2__ -O5 xxhash3.c hash.c swiss.c -o profile
./profile
wait
#gcov -a -b -c -g profile-profiling.c profile-hash.c profile-xxhash3.c
gcovr --html-nested covr.html
qutebrowser covr.html 2> /dev/null
