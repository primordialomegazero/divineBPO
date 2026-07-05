// PHI-RATE-LIMITER — phi-decay rate limiting
// Protects against DDoS, abuse

#pragma once
#include "phi_constants.h"
#include <map>
#include <string>
#include <chrono>

namespace divine {

class RateLimiter {
private:
    struct Bucket {
        int tokens;
        std::chrono::steady_clock::time_point last_refill;
    };
    
    std::map<std::string, Bucket> buckets;
    int max_tokens;
    double refill_rate; // tokens per second
    
public:
    RateLimiter(int max = 100, double rate = 10.0) 
        : max_tokens(max), refill_rate(rate) {}
    
    bool allow(const std::string& client_id) {
        auto& bucket = buckets[client_id];
        auto now = std::chrono::steady_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - bucket.last_refill).count() / 1000.0;
        
        // phi-decay refill
        bucket.tokens += static_cast<int>(elapsed * refill_rate * PHI_INV);
        if (bucket.tokens > max_tokens) bucket.tokens = max_tokens;
        bucket.last_refill = now;
        
        if (bucket.tokens > 0) {
            bucket.tokens--;
            return true;
        }
        return false;
    }
    
    void reset(const std::string& client_id) {
        buckets.erase(client_id);
    }
};

} // namespace divine
