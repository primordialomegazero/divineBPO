// PHI-MONITOR — Prometheus-compatible metrics
// Exposes /metrics endpoint for Grafana

#pragma once
#include <string>
#include <map>
#include <mutex>
#include <sstream>
#include <chrono>

namespace divine {

class Metrics {
private:
    std::map<std::string, double> gauges;
    std::map<std::string, int> counters;
    std::mutex mtx;
    std::chrono::steady_clock::time_point start_time;
    
public:
    Metrics() : start_time(std::chrono::steady_clock::now()) {}
    
    void gauge(const std::string& name, double value) {
        std::lock_guard<std::mutex> lock(mtx);
        gauges[name] = value;
    }
    
    void inc(const std::string& name) {
        std::lock_guard<std::mutex> lock(mtx);
        counters[name]++;
    }
    
    std::string render() {
        std::lock_guard<std::mutex> lock(mtx);
        std::stringstream ss;
        
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time).count();
        
        ss << "# HELP divine_bpo_uptime_seconds System uptime\n";
        ss << "# TYPE divine_bpo_uptime_seconds gauge\n";
        ss << "divine_bpo_uptime_seconds " << uptime << "\n";
        
        for (const auto& [name, val] : gauges) {
            ss << "# HELP divine_bpo_" << name << " Divine BPO metric\n";
            ss << "# TYPE divine_bpo_" << name << " gauge\n";
            ss << "divine_bpo_" << name << " " << val << "\n";
        }
        
        for (const auto& [name, val] : counters) {
            ss << "# HELP divine_bpo_" << name << "_total Divine BPO counter\n";
            ss << "# TYPE divine_bpo_" << name << "_total counter\n";
            ss << "divine_bpo_" << name << "_total " << val << "\n";
        }
        
        return ss.str();
    }
};

} // namespace divine
