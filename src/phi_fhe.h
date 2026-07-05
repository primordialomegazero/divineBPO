#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace divine {

class PhiFHE {
private:
    uint64_t rng_state;
    uint64_t chaos_key;
    
public:
    PhiFHE(uint64_t seed = 42) : rng_state(seed), chaos_key(seed ^ 0x9E3779B97F4A7C15ULL) {}
    
    uint64_t next_random() {
        rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
        return rng_state;
    }
    
    std::vector<uint64_t> encrypt(const std::string& plaintext) {
        std::vector<uint64_t> ciphertext;
        for (char c : plaintext) {
            uint64_t noise = next_random() & 0xFF;
            uint64_t encrypted = (static_cast<uint64_t>(c) ^ chaos_key) + noise;
            ciphertext.push_back(encrypted);
        }
        return ciphertext;
    }
    
    std::string decrypt(const std::vector<uint64_t>& ciphertext) {
        std::string plaintext;
        for (auto enc : ciphertext) {
            uint64_t noise = next_random() & 0xFF;
            uint64_t decrypted = (enc - noise) ^ chaos_key;
            plaintext += static_cast<char>(decrypted & 0xFF);
        }
        return plaintext;
    }
    
    std::vector<uint64_t> add(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
        std::vector<uint64_t> result;
        size_t len = std::max(a.size(), b.size());
        for (size_t i = 0; i < len; i++) {
            uint64_t va = (i < a.size()) ? a[i] : 0;
            uint64_t vb = (i < b.size()) ? b[i] : 0;
            result.push_back(va + vb);
        }
        return result;
    }
    
    bool equals(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
        if (a.size() != b.size()) return false;
        uint64_t diff = 0;
        for (size_t i = 0; i < a.size(); i++) diff |= (a[i] ^ b[i]);
        return diff == 0;
    }
};

} // namespace divine
