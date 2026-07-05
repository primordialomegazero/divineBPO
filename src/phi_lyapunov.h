// PHI-LYAPUNOV — Self-Healing System Monitor
// lambda = ln(phi) tracking, auto-scaling decisions

#pragma once
#include "phi_constants.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <deque>

namespace divine {

class LyapunovMonitor {
private:
    std::deque<double> metrics;
    size_t window_size;
    double target_lambda = 0.48121182505960347; // ln(phi)
    
public:
    LyapunovMonitor(size_t window = 100) : window_size(window) {}
    
    void record(double value) {
        metrics.push_back(value);
        if (metrics.size() > window_size) metrics.pop_front();
    }
    
    double current_lambda() const {
        if (metrics.size() < 2) return target_lambda;
        
        double sum = 0;
        for (size_t i = 1; i < metrics.size(); i++) {
            double ratio = metrics[i] / std::max(metrics[i-1], 0.001);
            sum += std::log(std::abs(ratio));
        }
        return sum / (metrics.size() - 1);
    }
    
    enum Action { NONE, DAMP, EXCITE, SCALE_UP, SCALE_DOWN };
    
    Action decide() const {
        double lambda = current_lambda();
        double drift = lambda - target_lambda;
        
        if (drift > 0.1) return DAMP;       // Too chaotic, stabilize
        if (drift < -0.1) return EXCITE;    // Too stable, increase entropy
        if (drift > 0.05) return SCALE_UP;  // Growing, add capacity
        if (drift < -0.05) return SCALE_DOWN; // Shrinking, reduce capacity
        return NONE;
    }
    
    void report() const {
        double lambda = current_lambda();
        auto action = decide();
        
        std::cout << "[LYAPUNOV] lambda=" << lambda << " (target=" << target_lambda << ") | ";
        
        switch (action) {
            case NONE:      std::cout << "STABLE"; break;
            case DAMP:      std::cout << "DAMPING (reduce chaos)"; break;
            case EXCITE:    std::cout << "EXCITING (increase entropy)"; break;
            case SCALE_UP:  std::cout << "SCALE UP (+1 node recommended)"; break;
            case SCALE_DOWN:std::cout << "SCALE DOWN (consolidate)"; break;
        }
        std::cout << std::endl;
    }
};

} // namespace divine
