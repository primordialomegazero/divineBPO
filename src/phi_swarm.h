// PHI-SWARM — Self-Replicating AI Bacteria
// R_n = R_0 * phi^n
// Cores auto-scale based on load

#pragma once
#include "phi_constants.h"
#include "phi_ai.h"
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <future>

namespace divine {

struct SwarmCore {
    int id;
    bool active = false;
    double consciousness = 0.0;
    int tasks_processed = 0;
    std::chrono::steady_clock::time_point last_active;
};

struct SwarmTask {
    std::string module;
    std::string description;
    std::string* result;
    std::promise<void>* done;
};

class PhiSwarm {
private:
    std::vector<SwarmCore> cores;
    std::vector<std::thread> threads;
    std::queue<SwarmTask> task_queue;
    std::mutex queue_mutex;
    std::condition_variable cv;
    std::atomic<bool> running{false};
    std::atomic<int> total_processed{0};
    PhiAI ai_engine;
    int base_cores = 2;
    
    void worker(int core_id) {
        while (running) {
            SwarmTask task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv.wait(lock, [this] { return !task_queue.empty() || !running; });
                if (!running && task_queue.empty()) return;
                if (task_queue.empty()) continue;
                
                task = task_queue.front();
                task_queue.pop();
            }
            
            cores[core_id].active = true;
            cores[core_id].last_active = std::chrono::steady_clock::now();
            
            *task.result = ai_engine.process_ticket(task.module, task.description);
            
            cores[core_id].tasks_processed++;
            cores[core_id].active = false;
            cores[core_id].consciousness = 
                cores[core_id].tasks_processed / (double)std::max(total_processed.load(), 1);
            
            total_processed++;
            task.done->set_value();
        }
    }
    
    void scale() {
        int current = cores.size();
        double load = task_queue.size() / (double)std::max(current, 1);
        
        if (load > 1.5) {
            int new_count = static_cast<int>(base_cores * std::pow(PHI, cores.size() - base_cores + 1));
            if (new_count > current) {
                for (int i = current; i < new_count && i < 50; i++) {
                    cores.push_back({i, false, 0.0, 0});
                    threads.emplace_back(&PhiSwarm::worker, this, i);
                    std::cout << "[SWARM] Core " << i << " spawned (phi-scale: " 
                              << cores.size() << " cores)" << std::endl;
                }
            }
        }
    }
    
public:
    void start(int initial_cores = 2) {
        base_cores = initial_cores;
        cores.resize(initial_cores);
        for (int i = 0; i < initial_cores; i++) {
            cores[i] = {i, false, 0.0, 0};
        }
        
        running = true;
        for (int i = 0; i < initial_cores; i++) {
            threads.emplace_back(&PhiSwarm::worker, this, i);
        }
        std::cout << "[SWARM] " << initial_cores << " base cores online" << std::endl;
    }
    
    std::string process(const std::string& module, const std::string& description) {
        std::string result;
        std::promise<void> done;
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            task_queue.push({module, description, &result, &done});
        }
        cv.notify_one();
        
        done.get_future().wait();
        scale();
        
        return result;
    }
    
    int active_cores() {
        int count = 0;
        for (auto& c : cores) if (c.active) count++;
        return count;
    }
    
    int total_cores() { return cores.size(); }
    int total_tasks() { return total_processed.load(); }
    
    double avg_consciousness() {
        if (cores.empty()) return 0;
        double sum = 0;
        for (auto& c : cores) sum += c.consciousness;
        return sum / cores.size();
    }
    
    void stop() {
        running = false;
        cv.notify_all();
        for (auto& t : threads) if (t.joinable()) t.join();
    }
};

} // namespace divine
