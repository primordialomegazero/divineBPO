// PHI-CONFIG — JSON config file loader
// No hardcoded values. Survives across versions.

#pragma once
#include "phi_storage.h"
#include <string>
#include <map>

namespace divine {

class PhiConfig {
private:
    PhiStorage& storage;
    std::map<std::string, std::string> cache;
    
public:
    PhiConfig(PhiStorage& s) : storage(s) {}
    
    void set(const std::string& key, const std::string& value) {
        cache[key] = value;
        storage.execute("INSERT OR REPLACE INTO config VALUES('" + key + "','" + value + "')");
    }
    
    std::string get(const std::string& key, const std::string& default_val = "") {
        if (cache.find(key) != cache.end()) return cache[key];
        std::string val = storage.query_value("SELECT value FROM config WHERE key='" + key + "'");
        if (val.empty()) return default_val;
        cache[key] = val;
        return val;
    }
    
    int get_int(const std::string& key, int default_val = 0) {
        std::string val = get(key);
        return val.empty() ? default_val : std::stoi(val);
    }
    
    bool get_bool(const std::string& key, bool default_val = false) {
        std::string val = get(key);
        return val.empty() ? default_val : (val == "true" || val == "1");
    }
    
    void load_defaults() {
        if (get("initialized") != "true") {
            set("app_name", "Divine BPO");
            set("version", "14.0");
            set("http_port", "8092");
            set("ws_port", "8093");
            set("mesh_port", "9000");
            set("swarm_cores", "2");
            set("ai_model", "llama3.2");
            set("rate_limit_max", "100");
            set("retry_attempts", "3");
            set("session_ttl", "86400");
            set("grpc_enabled", "false");
            set("debug_mode", "false");
            set("initialized", "true");
        }
    }
    
    void show() {
        std::cout << "[CONFIG] Current settings:" << std::endl;
        std::cout << "  app_name: " << get("app_name") << std::endl;
        std::cout << "  version: " << get("version") << std::endl;
        std::cout << "  http_port: " << get("http_port") << std::endl;
        std::cout << "  ws_port: " << get("ws_port") << std::endl;
        std::cout << "  mesh_port: " << get("mesh_port") << std::endl;
        std::cout << "  swarm_cores: " << get("swarm_cores") << std::endl;
        std::cout << "  ai_model: " << get("ai_model") << std::endl;
        std::cout << "  retry_attempts: " << get("retry_attempts") << std::endl;
    }
};

} // namespace divine
