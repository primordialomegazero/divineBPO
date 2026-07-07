// PHI-RATE-LIMIT — Per-IP Rate Limiting + Overflow Queue
// Token bucket algorithm, burst protection

#pragma once
#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <queue>

namespace divine {

struct TokenBucket {
    double tokens;
    std::chrono::steady_clock::time_point last_refill;
    int requests_served;
    int requests_queued;
};

class RateLimiter {
private:
    std::map<std::string, TokenBucket> buckets;
    std::mutex mtx;
    double rate;        // tokens per second
    double burst;       // max burst size
    int max_queue;      // max queue per IP
    
public:
    RateLimiter(double r = 100.0, double b = 200.0, int q = 1000) 
        : rate(r), burst(b), max_queue(q) {}
    
    bool allow(const std::string& ip) {
        std::unique_lock<std::mutex> lock(mtx);
        auto& bucket = buckets[ip];
        
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - bucket.last_refill).count();
        bucket.last_refill = now;
        
        // Refill tokens
        bucket.tokens = std::min(burst, bucket.tokens + elapsed * rate);
        
        if (bucket.tokens >= 1.0) {
            bucket.tokens -= 1.0;
            bucket.requests_served++;
            return true;
        }
        
        // Queue if under limit
        if (bucket.requests_queued < max_queue) {
            bucket.requests_queued++;
            return false;  // Client should retry
        }
        
        return false;  // Rate limited
    }
    
    int get_served(const std::string& ip) {
        std::unique_lock<std::mutex> lock(mtx);
        return buckets[ip].requests_served;
    }
};

// Request queue for overflow protection
class RequestQueue {
private:
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    size_t max_size;
    size_t dropped;
    
public:
    RequestQueue(size_t max = 10000) : max_size(max), dropped(0) {}
    
    bool enqueue(std::function<void()> task) {
        std::unique_lock<std::mutex> lock(mtx);
        if (tasks.size() >= max_size) {
            dropped++;
            return false;
        }
        tasks.push(task);
        return true;
    }
    
    bool dequeue(std::function<void()>& task) {
        std::unique_lock<std::mutex> lock(mtx);
        if (tasks.empty()) return false;
        task = tasks.front();
        tasks.pop();
        return true;
    }
    
    size_t size() { std::unique_lock<std::mutex> lock(mtx); return tasks.size(); }
    size_t total_dropped() const { return dropped; }
};

} // namespace divine
