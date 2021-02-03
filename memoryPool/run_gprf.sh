#!/bin/bash


# g++ -pg -g -O2 -o mempool  testcase3.cpp testalloc.cpp slabs.cpp -Wall
# ./mempool smem
# gprof ./mempool gmon.out > gprof.txt

 g++ -pg -lc -g -O0 -o mempool  simpletest.cpp slabs.cpp -Wall
 ./mempool
 gprof ./mempool gmon.out > gprof.txt