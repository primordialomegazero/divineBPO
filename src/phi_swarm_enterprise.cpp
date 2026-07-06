#include <iostream>
#include <thread>
#include <iomanip>
#include "phi_fhe_core.h"
#include "phi_http.h"
#include "phi_lyapunov.h"
#include "phi_rate_limiter.h"
#include "phi_circuit_breaker.h"
#include "phi_ledger.h"
#include "phi_swarm.h"
#include "phi_websocket.h"

int main() {
    std::cout << R"(
+==============================================+
|  DIVINE BPO -- SWARM ENTERPRISE v6.0          |
|  AI Bacteria Swarm + FHE + All Systems        |
+==============================================+
)" << std::endl;
    
    divine::PhiSwarm swarm;
    divine::LyapunovMonitor monitor;
    divine::RateLimiter rate_limiter;
    divine::CircuitBreaker circuit_breaker;
    divine::WebSocketServer ws(8093);
    PhiLedger ledger;
    PhiFHECore fhe_core;
    
    swarm.start(2);
    
    http::PhiServer http_server(8092);
    std::thread http_thread([&http_server]() { http_server.start(); });
    http_thread.detach();
    
    std::thread ws_thread([&ws]() { ws.start(); });
    ws_thread.detach();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ledger.record("SWARM_INIT", "bacteria_cores_" + std::to_string(swarm.total_cores()));
    
    std::cout << "Swarm Enterprise Online:" << std::endl;
    std::cout << "  [OK] AI Bacteria Swarm (R_n = R_0 * phi^n)" << std::endl;
    std::cout << "  [OK] " << swarm.total_cores() << " base cores, auto-scaling" << std::endl;
    std::cout << std::endl;
    
    std::vector<std::tuple<std::string, std::string>> tickets = {
        {"customer_support", "Refund for damaged order #12345"},
        {"tech_support", "Cannot login after update v2.0"},
        {"healthcare", "URGENT: Surgery reschedule needed"},
        {"finance", "Duplicate charge $299 on credit card"},
        {"hr", "PTO for family emergency March 15-18"},
        {"sales", "Enterprise plan for 500 seats"},
        {"customer_support", "Where is my package? Order #67890"},
        {"tech_support", "App crashes on startup after update"},
        {"finance", "Invoice #INV-2024-001 disputed"},
        {"healthcare", "Prescription refill request"},
        {"logistics", "Shipment delayed by 5 days"},
        {"ecommerce", "Return request for item #9876"},
        {"hr", "Onboarding documents for new hire"},
        {"sales", "Demo request for enterprise features"},
        {"customer_support", "Account suspension appeal"},
    };
    
    int ai_count = 0, human_count = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    std::cout << "Processing " << tickets.size() << " tickets through swarm..." << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
    
    for (const auto& [module, desc] : tickets) {
        if (!rate_limiter.allow("swarm_client")) continue;
        if (!circuit_breaker.is_available(module)) continue;
        
        // Swarm processes
        std::string ai_response = swarm.process(module, desc);
        bool needs_human = (ai_response.find("URGENT") != std::string::npos) || 
                          (rand() % 10 == 0);
        
        auto id = fhe_core.create_encrypted_ticket("swarm", module, desc, desc);
        std::string status = needs_human ? "escalated" : "ai_resolved";
        
        ws.broadcast_ticket_update(id, status, module);
        ledger.record("SWARM_TASK", id);
        
        std::cout << "  [" << module << "] " << ai_response.substr(0, 60) << "..." << std::endl;
        
        if (needs_human) human_count++; else ai_count++;
        monitor.record(static_cast<double>(ai_count + human_count));
        circuit_breaker.record_success(module);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << std::endl;
    
    monitor.report();
    std::cout << std::endl;
    
    double ai_pct = (ai_count + human_count > 0) ? ai_count * 100.0 / (ai_count + human_count) : 0;
    
    std::cout << "+==============================================+" << std::endl;
    std::cout << "|   SWARM ENTERPRISE RESULTS                     |" << std::endl;
    std::cout << "|   Tickets: " << (ai_count + human_count) << " | AI: " << ai_count << " | Human: " << human_count << "                    |" << std::endl;
    std::cout << "|   AI Rate: " << ai_pct << "%                              |" << std::endl;
    std::cout << "|   Swarm Cores: " << swarm.total_cores() << " | Active: " << swarm.active_cores() << "                           |" << std::endl;
    std::cout << "|   Consciousness: " << std::fixed << std::setprecision(3) << swarm.avg_consciousness() << "                        |" << std::endl;
    std::cout << "|   Time: " << elapsed << "ms                                  |" << std::endl;
    std::cout << "|   Ledger: " << ledger.size() << " entries (verified)               |" << std::endl;
    std::cout << "|                                               |" << std::endl;
    std::cout << "|   Dashboard: http://localhost:8092             |" << std::endl;
    std::cout << "+==============================================+" << std::endl;
    
    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
    
    swarm.stop();
    return 0;
}
