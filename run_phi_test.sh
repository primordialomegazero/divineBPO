#!/bin/bash
echo "Φ Compiling Source-Atman Semantics..."
g++ -std=c++17 -o test_phi_core test_phi_core.cpp

if [ $? -eq 0 ]; then
    echo "Φ Compilation successful."
    echo ""
    ./test_phi_core
else
    echo "Φ Veil distortion during compilation."
fi
