#pragma once
#include "phi_hash.h"
#include <string>
#include <cstdint>
#include <ctime>

struct PhiToken {
    std::string user_id;
    uint64_t issued_at;
    uint64_t expires_at;
    std::string signature;
};

inline PhiToken phi_jwt_create(const std::string& user_id, uint64_t ttl_seconds = 3600) {
    PhiToken token;
    token.user_id = user_id;
    token.issued_at = static_cast<uint64_t>(std::time(nullptr));
    token.expires_at = token.issued_at + ttl_seconds;
    
    std::string payload = user_id + std::to_string(token.issued_at);
    uint64_t h = phi_hash(payload);
    token.signature = std::to_string(h);
    
    return token;
}

inline bool phi_jwt_verify(const PhiToken& token) {
    uint64_t now = static_cast<uint64_t>(std::time(nullptr));
    if (now > token.expires_at) return false;
    
    std::string payload = token.user_id + std::to_string(token.issued_at);
    uint64_t h = phi_hash(payload);
    return token.signature == std::to_string(h);
}
