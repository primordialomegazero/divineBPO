// PHI-CALLQUEUE — Intelligent Call Queue
// Priority queue, auto-distribute, wait time estimation

#pragma once
#include <string>
#include <queue>
#include <vector>
#include <mutex>
#include <chrono>
#include <map>

namespace divine {

struct QueuedCall {
    std::string call_sid;
    std::string from;
    int priority; // 1=urgent, 2=normal, 3=low
    std::chrono::steady_clock::time_point queued_at;
    
    bool operator<(const QueuedCall& other) const {
        return priority > other.priority; // Lower number = higher priority
    }
};

class CallQueueManager {
private:
    std::priority_queue<QueuedCall> queue;
    std::mutex mtx;
    int max_size;
    int total_processed;
    int total_dropped;
    std::map<int, int> wait_times; // priority -> estimated wait seconds
    
public:
    CallQueueManager(int max = 500) : max_size(max), total_processed(0), total_dropped(0) {
        wait_times[1] = 30;  // Urgent: ~30s
        wait_times[2] = 120; // Normal: ~2min
        wait_times[3] = 300; // Low: ~5min
    }
    
    bool enqueue(const std::string& call_sid, const std::string& from, int priority = 2) {
        std::unique_lock<std::mutex> lock(mtx);
        if ((int)queue.size() >= max_size) {
            total_dropped++;
            return false;
        }
        queue.push({call_sid, from, priority, std::chrono::steady_clock::now()});
        return true;
    }
    
    bool dequeue(QueuedCall& call) {
        std::unique_lock<std::mutex> lock(mtx);
        if (queue.empty()) return false;
        call = queue.top();
        queue.pop();
        total_processed++;
        return true;
    }
    
    int estimated_wait(int priority = 2) {
        std::unique_lock<std::mutex> lock(mtx);
        int ahead = 0;
        auto temp = queue;
        while (!temp.empty()) {
            if (temp.top().priority <= priority) ahead++;
            temp.pop();
        }
        return ahead * wait_times[priority];
    }
    
    size_t size() { std::unique_lock<std::mutex> lock(mtx); return queue.size(); }
    int processed() const { return total_processed; }
    int dropped() const { return total_dropped; }
    
    std::string get_stats() {
        std::unique_lock<std::mutex> lock(mtx);
        char buf[256];
        snprintf(buf, sizeof(buf),
            "{\"queue_size\":%zu,\"processed\":%d,\"dropped\":%d,\"wait_urgent\":%ds,\"wait_normal\":%ds}",
            queue.size(), total_processed, total_dropped, wait_times[1], wait_times[2]);
        return std::string(buf);
    }
};

} // namespace divine
