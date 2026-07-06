// PHI-ERROR — Error handling + Retry logic
// Exponential backoff with phi-scaling
// Graceful degradation per service

#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <functional>
#include <map>

namespace divine {

struct RetryConfig {
    int max_attempts = 3;
    int base_delay_ms = 1618; // phi * 1000
    double backoff_factor = 1.618; // phi
};

class ErrorHandler {
private:
    std::map<std::string, int> failure_counts;
    std::map<std::string, bool> service_status;
    RetryConfig config;
    
public:
    ErrorHandler(const RetryConfig& cfg = RetryConfig()) : config(cfg) {}
    
    enum Result { OK, RETRY, FAIL, DEGRADED };
    
    Result execute(const std::string& service,
                   std::function<bool()> operation,
                   const std::string& fallback_msg = "") {
        
        if (service_status[service]) {
            std::cerr << "[ERROR] " << service << " is DOWN, using degraded mode" << std::endl;
            return DEGRADED;
        }
        
        for (int attempt = 1; attempt <= config.max_attempts; attempt++) {
            try {
                if (operation()) {
                    failure_counts[service] = 0;
                    return OK;
                }
            } catch (...) {
                // Operation threw
            }
            
            if (attempt < config.max_attempts) {
                int delay = static_cast<int>(config.base_delay_ms * 
                            std::pow(config.backoff_factor, attempt - 1));
                std::cerr << "[RETRY] " << service << " attempt " << attempt 
                         << "/" << config.max_attempts << " failed, retrying in " 
                         << delay << "ms" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
        }
        
        failure_counts[service]++;
        
        if (failure_counts[service] >= 5) {
            service_status[service] = true;
            std::cerr << "[ERROR] " << service << " marked DOWN after " 
                     << failure_counts[service] << " failures" << std::endl;
        }
        
        return FAIL;
    }
    
    template<typename T>
    T execute_with_fallback(const std::string& service,
                            std::function<T()> primary,
                            std::function<T()> fallback) {
        if (service_status[service]) {
            return fallback();
        }
        
        for (int attempt = 1; attempt <= config.max_attempts; attempt++) {
            try {
                T result = primary();
                failure_counts[service] = 0;
                return result;
            } catch (...) {}
            
            if (attempt < config.max_attempts) {
                int delay = static_cast<int>(config.base_delay_ms * 
                            std::pow(config.backoff_factor, attempt - 1));
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
        }
        
        failure_counts[service]++;
        if (failure_counts[service] >= 5) {
            service_status[service] = true;
        }
        return fallback();
    }
    
    bool is_service_up(const std::string& service) {
        return !service_status[service];
    }
    
    void reset_service(const std::string& service) {
        service_status[service] = false;
        failure_counts[service] = 0;
    }
    
    int failure_count(const std::string& service) {
        return failure_counts[service];
    }
    
    void health_report() {
        std::cout << "[HEALTH] Services:" << std::endl;
        for (const auto& [svc, down] : service_status) {
            std::cout << "  " << svc << ": " << (down ? "DOWN" : "UP") 
                     << " (failures: " << failure_counts[svc] << ")" << std::endl;
        }
    }
};

} // namespace divine
