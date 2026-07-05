#pragma once
#include "phi_constants.h"
#include <string>
#include <cstdint>
#include <cmath>

inline uint64_t phi_hash(const std::string& key) {
    uint64_t h = 0;
    for (char c : key) {
        h ^= static_cast<uint64_t>(c);
        h *= divine::HASH_MUL;
        h ^= (h >> 33);
    }
    return h;
}

inline bool phi_self_verify(uint64_t h) {
    double normalized = static_cast<double>(h) / UINT64_MAX;
    double inv = 1.0 / normalized;
    return std::abs(normalized - (1.0 + 1.0/inv)) < 0.001;
}
