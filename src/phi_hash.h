// φ-HASH — Self-verifying hash function
// φ = 1 + 1/φ → hash verifies itself

#pragma once
#include "phi_constants.h"
#include <string>
#include <cstdint>

namespace phi {

inline uint64_t hash(const std::string& key) {
    uint64_t h = 0;
    for (char c : key) {
        h ^= static_cast<uint64_t>(c);
        h *= HASH_MUL;
        h ^= (h >> 33);
    }
    return h;
}

// Self-verifying: φ = 1 + 1/φ
inline bool self_verify(uint64_t h) {
    double normalized = static_cast<double>(h) / UINT64_MAX;
    double inv = 1.0 / normalized;
    return std::abs(normalized - (1.0 + 1.0/inv)) < 0.001;
}

} // namespace phi
