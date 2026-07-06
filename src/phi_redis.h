// PHI-REDIS — Key-Value Store Bridge
// Connects to local Redis. Falls back to in-memory if unavailable.

#pragma once
#include <string>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <array>

namespace divine {

class RedisDB {
private:
    bool available;
    
    std::string exec(const std::string& cmd) {
        std::string full_cmd = "redis-cli " + cmd + " 2>/dev/null";
        std::array<char, 256> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(
            popen(full_cmd.c_str(), "r"), pclose);
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        if (!result.empty() && result.back() == '\n') result.pop_back();
        return result;
    }
    
public:
    RedisDB() {
        available = (system("redis-cli ping 2>/dev/null | grep -q PONG") == 0);
    }
    
    bool is_available() const { return available; }
    
    void set(const std::string& key, const std::string& value) {
        if (!available) return;
        exec("SET " + key + " '" + value + "'");
    }
    
    std::string get(const std::string& key) {
        if (!available) return "";
        return exec("GET " + key);
    }
    
    void incr(const std::string& key) {
        if (!available) return;
        exec("INCR " + key);
    }
    
    void expire(const std::string& key, int seconds) {
        if (!available) return;
        exec("EXPIRE " + key + " " + std::to_string(seconds));
    }
    
    void hset(const std::string& hash, const std::string& field, const std::string& value) {
        if (!available) return;
        exec("HSET " + hash + " " + field + " '" + value + "'");
    }
    
    std::string hget(const std::string& hash, const std::string& field) {
        if (!available) return "";
        return exec("HGET " + hash + " " + field);
    }
    
    void lpush(const std::string& list, const std::string& value) {
        if (!available) return;
        exec("LPUSH " + list + " '" + value + "'");
    }
    
    std::string rpop(const std::string& list) {
        if (!available) return "";
        return exec("RPOP " + list);
    }
};

} // namespace divine
