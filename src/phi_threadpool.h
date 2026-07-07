// PHI-THREADPOOL — High-Performance Worker Pool
// 10k concurrent connections, non-blocking

#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <memory>

namespace divine {

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> running;
    size_t active_tasks;

public:
    ThreadPool(size_t threads = 0) : running(true), active_tasks(0) {
        if (threads == 0) {
            threads = std::thread::hardware_concurrency() * 2;
            if (threads < 4) threads = 4;
        }
        
        for (size_t i = 0; i < threads; i++) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] {
                            return !this->running || !this->tasks.empty();
                        });
                        if (!this->running && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                        active_tasks++;
                    }
                    task();
                    active_tasks--;
                }
            });
        }
    }

    template<class F>
    void enqueue(F&& task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::forward<F>(task));
        }
        condition.notify_one();
    }

    size_t pending() const { return tasks.size(); }
    size_t active() const { return active_tasks; }
    size_t size() const { return workers.size(); }

    ~ThreadPool() {
        running = false;
        condition.notify_all();
        for (auto& worker : workers) {
            if (worker.joinable()) worker.join();
        }
    }
};

// Global thread pool
inline std::unique_ptr<ThreadPool> g_pool;

} // namespace divine
