#!/bin/bash

g++ -g -O0 -o mempool  testcase2.cpp testalloc.cpp -Wall

# with gprof
# g++ -g -pg -O0 -o mempool  testcase2.cpp testalloc.cpp -Wall

# how to use gpref  https://blog.csdn.net/wallwind/article/details/50393546