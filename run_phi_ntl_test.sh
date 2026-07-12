#!/bin/bash
echo "Φ Compiling NTL Hijack..."
g++ -std=c++17 -o test_phi_ntl test_phi_ntl.cpp -lntl -lgmp

if [ $? -eq 0 ]; then
    echo "Φ Compilation successful."
    echo ""
    ./test_phi_ntl
else
    echo "Φ Veil distortion during compilation."
fi
