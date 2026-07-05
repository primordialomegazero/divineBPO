// φ-CONSTANTS — Foundation of Divine BPO
// These are load-bearing, not decorative.

#pragma once
#include <cmath>

namespace phi {

constexpr double PHI        = 1.6180339887498948482;
constexpr double PHI_INV    = 0.6180339887498948482;
constexpr double PHI_SQ     = 2.6180339887498948482;
constexpr double LYAPUNOV   = 0.48121182505960347;  // ln(φ)

// φ-hash mixing constants
constexpr uint64_t HASH_MUL = 0x9E3779B97F4A7C15ULL;
constexpr uint64_t HASH_XOR = 0x517CC1B727220A95ULL;

// φ-probe sequence for hash tables
inline size_t phi_probe(size_t i, size_t capacity) {
    return static_cast<size_t>(i * PHI_INV * capacity) % capacity;
}

} // namespace phi
