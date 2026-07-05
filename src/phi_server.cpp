#include <iostream>
#include <thread>
#include "phi_core.h"
#include "phi_http.h"

int main() {
    std::cout << R"(
+==============================================+
|          DIVINE BPO -- v1.0                   |
|   phi-Gateway + phi-Database + phi-Auth       |
|   90% AI | 10% Human | Unlimited Scale        |
+==============================================+
)" << std::endl;
    
    // Start HTTP server in background
    http::PhiServer server(8092);
    std::thread http_thread([&server]() {
        server.start();
    });
    http_thread.detach();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Run BPO core demo
    PhiCore core;
    
    std::cout << "Modules: ";
    for (const auto& m : core.modules) std::cout << m << " ";
    std::cout << std::endl << std::endl;
    
    std::vector<std::pair<std::string, std::string>> tickets = {
        {"customer_support", "Order #12345 not delivered"},
        {"tech_support", "Login failure on mobile app"},
        {"healthcare", "Reschedule appointment request"},
        {"finance", "Duplicate charge on credit card"},
        {"hr", "PTO request for March 15-18"},
        {"sales", "Enterprise plan inquiry"},
        {"logistics", "Shipment #9876 delayed"},
        {"ecommerce", "Return request for item #6789"},
        {"customer_support", "Refund status check"},
        {"tech_support", "App crashes on startup"},
    };
    
    int ai = 0, human = 0;
    
    std::cout << "Processing tickets..." << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
    
    for (const auto& [module, desc] : tickets) {
        auto id = core.create_ticket("client_001", module, module, desc);
        auto result = core.process_ticket(id);
        auto ticket = core.get_ticket(id);
        
        std::cout << "  " << id << " | " << module << std::endl;
        std::cout << "    " << result << std::endl;
        
        if (ticket->needs_human) human++; else ai++;
    }
    
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << std::endl;
    std::cout << "Results: " << ai << " AI | " << human << " Human | " 
              << (ai * 100.0 / (ai + human)) << "% AI" << std::endl;
    std::cout << std::endl;
    std::cout << "Dashboard: http://localhost:8092" << std::endl;
    std::cout << "API:       http://localhost:8092/api/metrics" << std::endl;
    std::cout << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    // Keep running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
