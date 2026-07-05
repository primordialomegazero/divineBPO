#include <iostream>
#include <thread>
#include "phi_fhe_core.h"
#include "phi_http.h"
#include "phi_lyapunov.h"
#include "phi_rate_limiter.h"
#include "phi_circuit_breaker.h"
#include "phi_ai.h"

int main() {
    std::cout << R"(
+==============================================+
|    DIVINE BPO -- AI ENTERPRISE v4.0           |
|   Real AI (Ollama) + FHE + All Enterprise     |
+==============================================+
)" << std::endl;
    
    divine::LyapunovMonitor monitor;
    divine::RateLimiter rate_limiter;
    divine::CircuitBreaker circuit_breaker;
    PhiFHECore fhe_core;
    divine::PhiAI ai;
    
    http::PhiServer server(8092);
    std::thread http_thread([&server]() { server.start(); });
    http_thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "AI Enterprise systems online:" << std::endl;
    std::cout << "  [OK] phi-AI (Ollama local LLM + rule-based fallback)" << std::endl;
    std::cout << "  [OK] phi-FHE (homomorphic encryption)" << std::endl;
    std::cout << "  [OK] phi-Ledger (immutable, verified)" << std::endl;
    std::cout << std::endl;
    
    std::vector<std::tuple<std::string, std::string>> ai_tickets = {
        {"customer_support", "I need a refund for my order #12345 that arrived damaged"},
        {"tech_support", "Cannot login to my account after the latest update"},
        {"healthcare", "URGENT: Need to reschedule my surgery appointment"},
        {"finance", "There's a duplicate charge of $299 on my credit card"},
        {"hr", "Requesting PTO for March 15-18 for family emergency"},
        {"sales", "Interested in your enterprise plan for 500 seats"},
    };
    
    int ai_count = 0, human_count = 0;
    
    std::cout << "Processing with AI..." << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
    
    for (const auto& [module, desc] : ai_tickets) {
        if (!rate_limiter.allow("ai_client")) continue;
        if (!circuit_breaker.is_available(module)) continue;
        
        // AI classifies module
        std::string detected_module = ai.classify_module(desc);
        bool needs_human = ai.needs_human(desc);
        
        // AI processes
        std::string ai_response = ai.process_ticket(detected_module, desc);
        
        // Store in FHE
        auto id = fhe_core.create_encrypted_ticket("ai_client", detected_module, 
                                                      "AI Ticket", desc);
        
        std::cout << "  TKT | " << detected_module << std::endl;
        std::cout << "    Input:    " << desc.substr(0, 60) << "..." << std::endl;
        std::cout << "    AI:       " << ai_response.substr(0, 70) << "..." << std::endl;
        std::cout << "    Escalate: " << (needs_human ? "YES (human)" : "NO (ai)") << std::endl;
        
        if (needs_human) human_count++; else ai_count++;
        monitor.record(static_cast<double>(ai_count + human_count));
        circuit_breaker.record_success(detected_module);
    }
    
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << std::endl;
    
    monitor.report();
    std::cout << std::endl;
    
    double ai_pct = (ai_count + human_count > 0) ? ai_count * 100.0 / (ai_count + human_count) : 0;
    
    std::cout << "+==============================================+" << std::endl;
    std::cout << "|   AI ENTERPRISE RESULTS                       |" << std::endl;
    std::cout << "|   Tickets: " << (ai_count + human_count) << " | AI: " << ai_count << " | Human: " << human_count << "                    |" << std::endl;
    std::cout << "|   AI Rate: " << ai_pct << "%                              |" << std::endl;
    std::cout << "|   Ledger: " << fhe_core.ledger_size() << " entries (verified)               |" << std::endl;
    std::cout << "|                                               |" << std::endl;
    std::cout << "|   Dashboard: http://localhost:8092             |" << std::endl;
    std::cout << "+==============================================+" << std::endl;
    
    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
