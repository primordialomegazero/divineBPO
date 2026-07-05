// PHI-CIRCUIT-BREAKER — Fault isolation
// Prevents cascade failures

#pragma once
#include <string>
#include <map>
#include <chrono>

namespace divine {

class CircuitBreaker {
public:
    enum State { CLOSED, OPEN, HALF_OPEN };
    
private:
    struct Service {
        State state = CLOSED;
        int failures = 0;
        int failure_threshold = 5;
        std::chrono::steady_clock::time_point last_failure;
        std::chrono::steady_clock::time_point opened_at;
        int reset_timeout_ms = 30000; // 30 seconds
    };
    
    std::map<std::string, Service> services;
    
public:
    bool is_available(const std::string& service_name) {
        auto& svc = services[service_name];
        auto now = std::chrono::steady_clock::now();
        
        switch (svc.state) {
            case CLOSED:
                return true;
                
            case OPEN: {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - svc.opened_at).count();
                if (elapsed > svc.reset_timeout_ms) {
                    svc.state = HALF_OPEN;
                    return true;
                }
                return false;
            }
            
            case HALF_OPEN:
                return true;
        }
        return true;
    }
    
    void record_success(const std::string& service_name) {
        auto& svc = services[service_name];
        svc.failures = 0;
        svc.state = CLOSED;
    }
    
    void record_failure(const std::string& service_name) {
        auto& svc = services[service_name];
        svc.failures++;
        svc.last_failure = std::chrono::steady_clock::now();
        
        if (svc.failures >= svc.failure_threshold) {
            svc.state = OPEN;
            svc.opened_at = std::chrono::steady_clock::now();
        }
    }
    
    State state(const std::string& service_name) const {
        auto it = services.find(service_name);
        if (it == services.end()) return CLOSED;
        return it->second.state;
    }
};

} // namespace divine
