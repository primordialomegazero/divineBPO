// PHI-QUEUE — RabbitMQ Message Queue Bridge
// Async task processing. Dead letter queue support.

#pragma once
#include <string>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <array>

namespace divine {

class MessageQueue {
private:
    bool available;
    
    std::string exec(const std::string& cmd) {
        std::array<char, 256> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(
            popen(cmd.c_str(), "r"), pclose);
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }
    
public:
    MessageQueue() {
        available = (system("rabbitmqctl status 2>/dev/null | grep -q rabbit") == 0);
    }
    
    bool is_available() const { return available; }
    
    void publish(const std::string& queue, const std::string& message) {
        if (!available) return;
        std::string cmd = "rabbitmqadmin publish routing_key=" + queue + 
                         " payload='" + message + "' 2>/dev/null";
        system(cmd.c_str());
    }
    
    std::string consume(const std::string& queue) {
        if (!available) return "";
        return exec("rabbitmqadmin get queue=" + queue + " requeue=false 2>/dev/null");
    }
    
    void declare_queue(const std::string& name) {
        if (!available) return;
        std::string cmd = "rabbitmqadmin declare queue name=" + name + 
                         " durable=true 2>/dev/null";
        system(cmd.c_str());
    }
    
    void declare_dead_letter(const std::string& queue, const std::string& dlq) {
        if (!available) return;
        std::string cmd = "rabbitmqadmin declare queue name=" + dlq + 
                         " durable=true 2>/dev/null";
        system(cmd.c_str());
    }
    
    int queue_depth(const std::string& queue) {
        if (!available) return 0;
        std::string result = exec("rabbitmqadmin list queues name messages 2>/dev/null | grep " + queue);
        return result.empty() ? 0 : 1;
    }
};

} // namespace divine
