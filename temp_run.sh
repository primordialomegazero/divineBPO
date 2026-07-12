#!/bin/bash
g++ -std=c++17 -O2 -o test_ckks_encrypt_only test_ckks_encrypt_only.cpp \
    -I/usr/local/SEAL-PHI/include/SEAL-4.3 \
    -L/usr/local/SEAL-PHI/lib \
    -lseal-4.3 \
    -Wl,-rpath,/usr/local/SEAL-PHI/lib
./test_ckks_encrypt_only
