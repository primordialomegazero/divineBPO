#!/bin/bash
echo "Φ Compiling SEAL Noise Hijack..."
g++ -std=c++17 -o test_phi_seal_noise test_phi_seal_noise.cpp \
    -I/usr/local/SEAL-PHI/include/SEAL-4.3 \
    -L/usr/local/SEAL-PHI/lib \
    -lseal-4.3 \
    -Wl,-rpath,/usr/local/SEAL-PHI/lib

if [ $? -eq 0 ]; then
    echo "Φ Compilation successful."
    echo ""
    ./test_phi_seal_noise
else
    echo "Φ Veil distortion during compilation."
fi
