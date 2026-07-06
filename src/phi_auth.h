#pragma once
#include "phi_storage.h"
#include "phi_hash.h"
#include <string>
#include <ctime>

namespace divine {

class PhiAuth {
private:
    PhiStorage& storage;

    std::string make_token(const std::string& user) {
        return "phi-sess-" + std::to_string(phi_hash(user + std::to_string(std::time(nullptr))));
    }

public:
    PhiAuth(PhiStorage& s) : storage(s) {}

    std::string login(const std::string& username) {
        std::string existing = storage.query_value(
            "SELECT token FROM sessions WHERE user_id='" + username + 
            "' AND expires_at > " + std::to_string(std::time(nullptr)));
        if (!existing.empty()) return existing;

        std::string token = make_token(username);
        auto now = std::time(nullptr);
        storage.execute("INSERT INTO sessions VALUES('" + token + "','" + username + "'," + 
                       std::to_string(now) + "," + std::to_string(now + 86400) + ")");
        return token;
    }

    bool validate(const std::string& token) {
        return !storage.query_value(
            "SELECT user_id FROM sessions WHERE token='" + token + 
            "' AND expires_at > " + std::to_string(std::time(nullptr))).empty();
    }

    std::string whois(const std::string& token) {
        return storage.query_value("SELECT user_id FROM sessions WHERE token='" + token + "'");
    }

    void logout(const std::string& token) {
        storage.execute("DELETE FROM sessions WHERE token='" + token + "'");
    }

    int session_count() {
        return storage.query_int("SELECT COUNT(*) FROM sessions WHERE expires_at > " + 
                                std::to_string(std::time(nullptr)));
    }
};

} // namespace divine
