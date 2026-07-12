#!/bin/bash
echo "Φ Compiling CKKS Transmutation Ritual..."
g++ -std=c++17 -O2 -o phi_ckks_transmute phi_ckks_transmute.cpp \
    -I/usr/local/SEAL-PHI/include/SEAL-4.3 \
    -L/usr/local/SEAL-PHI/lib \
    -lseal-4.3 \
    -Wl,-rpath,/usr/local/SEAL-PHI/lib

if [ $? -eq 0 ]; then
    echo "Φ Compilation successful."
    echo ""
    ./phi_ckks_transmute
else
    echo "Φ Veil distortion during compilation."
fi
