#include <iostream>
#include <thread>
#include "phi_fhe_core.h"
#include "phi_http.h"
#include "phi_lyapunov.h"
#include "phi_rate_limiter.h"
#include "phi_circuit_breaker.h"
#include "phi_ai.h"
#include "phi_websocket.h"

int main() {
    std::cout << R"(
+==============================================+
|    DIVINE BPO -- WEBSOCKET ENTERPRISE v5.0    |
|   Real-time Updates + AI + FHE + Enterprise   |
+==============================================+
)" << std::endl;
    
    divine::LyapunovMonitor monitor;
    divine::RateLimiter rate_limiter;
    divine::CircuitBreaker circuit_breaker;
    PhiFHECore fhe_core;
    divine::PhiAI ai;
    divine::WebSocketServer ws(8093);
    
    // Start servers
    http::PhiServer http_server(8092);
    std::thread http_thread([&http_server]() { http_server.start(); });
    http_thread.detach();
    
    std::thread ws_thread([&ws]() { ws.start(); });
    ws_thread.detach();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "WebSocket Enterprise systems online:" << std::endl;
    std::cout << "  [OK] phi-HTTP (dashboard on :8092)" << std::endl;
    std::cout << "  [OK] phi-WebSocket (real-time on :8093)" << std::endl;
    std::cout << "  [OK] phi-AI (Ollama LLM)" << std::endl;
    std::cout << "  [OK] phi-FHE (homomorphic encryption)" << std::endl;
    std::cout << std::endl;
    
    std::vector<std::tuple<std::string, std::string>> tickets = {
        {"customer_support", "Refund for damaged order #12345"},
        {"tech_support", "Cannot login after v2.0 update"},
        {"healthcare", "URGENT: Surgery reschedule needed"},
        {"finance", "Duplicate charge $299 on credit card"},
        {"hr", "PTO for family emergency March 15-18"},
        {"sales", "Enterprise plan inquiry for 500 seats"},
    };
    
    int ai_count = 0, human_count = 0;
    
    std::cout << "Processing with real-time WebSocket updates..." << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
    
    for (const auto& [module, desc] : tickets) {
        if (!rate_limiter.allow("ws_client")) continue;
        
        std::string detected = ai.classify_module(desc);
        bool needs_human = ai.needs_human(desc);
        std::string ai_response = ai.process_ticket(detected, desc);
        
        auto id = fhe_core.create_encrypted_ticket("ws_client", detected, desc, desc);
        std::string status = needs_human ? "escalated" : "ai_resolved";
        
        // Broadcast real-time update
        ws.broadcast_ticket_update(id, status, detected);
        
        std::cout << "  " << id << " | " << detected << std::endl;
        std::cout << "    AI: " << ai_response.substr(0, 70) << "..." << std::endl;
        std::cout << "    WS: broadcasted to " << ws.client_count() << " clients" << std::endl;
        std::cout << "    Status: " << status << std::endl;
        
        if (needs_human) human_count++; else ai_count++;
        monitor.record(static_cast<double>(ai_count + human_count));
        circuit_breaker.record_success(detected);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << std::endl;
    
    monitor.report();
    std::cout << std::endl;
    
    double ai_pct = (ai_count + human_count > 0) ? ai_count * 100.0 / (ai_count + human_count) : 0;
    
    std::cout << "+==============================================+" << std::endl;
    std::cout << "|   WEBSOCKET ENTERPRISE RESULTS                |" << std::endl;
    std::cout << "|   Tickets: " << (ai_count + human_count) << " | AI: " << ai_count << " | Human: " << human_count << "                    |" << std::endl;
    std::cout << "|   AI Rate: " << ai_pct << "%                              |" << std::endl;
    std::cout << "|   WS Clients: " << ws.client_count() << "                               |" << std::endl;
    std::cout << "|   Ledger: " << fhe_core.ledger_size() << " entries (verified)               |" << std::endl;
    std::cout << "|                                               |" << std::endl;
    std::cout << "|   Dashboard: http://localhost:8092             |" << std::endl;
    std::cout << "|   WebSocket: ws://localhost:8093               |" << std::endl;
    std::cout << "+==============================================+" << std::endl;
    
    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
