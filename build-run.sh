#!/bin/bash

rm -rf build/
mkdir build/
for M in 0 1; do
for I in 0 1; do
for T in 0 1; do
    if [ $I == 0 ] && [ $T == 0 ]; then
        continue
    fi
    exe=./build/memtest-m$M-i$I-t$T
    macro="-DMAGIC_NOT_ZERO=$M -DWARMUP_INITMAGIC=$I -DWARMUP_TESTFUNC=$T"
    gcc -O3 -fno-tree-loop-distribute-patterns -DMAGIC_NOT_ZERO=$M -DWARMUP_INITMAGIC=$I -DWARMUP_TESTFUNC=$T memtest.c -o $exe
    $exe
done
done
done
