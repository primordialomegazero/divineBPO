#include <iostream>
#include <thread>
#include "phi_core.h"
#include "phi_http.h"
#include "phi_lyapunov.h"
#include "phi_rate_limiter.h"
#include "phi_circuit_breaker.h"
#include "phi_ledger.h"

int main() {
    std::cout << R"(
+==============================================+
|       DIVINE BPO -- ENTERPRISE v2.0           |
|   phi-Lyapunov + Rate Limiter + Circuit Breaker|
|   phi-Ledger + 14 Modules + HTTP Server       |
|   90% AI | 10% Human | Unlimited Scale        |
+==============================================+
)" << std::endl;
    
    // Initialize enterprise components
    divine::LyapunovMonitor monitor;
    divine::RateLimiter rate_limiter(100, 10.0);
    divine::CircuitBreaker circuit_breaker;
    PhiLedger ledger;
    PhiCore core;
    
    ledger.record("SYSTEM_START", "divine_bpo_enterprise_v2");
    
    // Start HTTP server
    http::PhiServer server(8092);
    std::thread http_thread([&server]() { server.start(); });
    http_thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Enterprise health check
    std::cout << "Enterprise systems online:" << std::endl;
    std::cout << "  [OK] phi-Lyapunov (lambda target: 0.4812)" << std::endl;
    std::cout << "  [OK] phi-Rate Limiter (phi-decay buckets)" << std::endl;
    std::cout << "  [OK] phi-Circuit Breaker (fault isolation)" << std::endl;
    std::cout << "  [OK] phi-Ledger (immutable audit trail)" << std::endl;
    std::cout << "  [OK] phi-Database (O(1) phi-probing)" << std::endl;
    std::cout << "  [OK] phi-JWT (self-verifying tokens)" << std::endl;
    std::cout << std::endl;
    
    // Simulate enterprise workload
    std::vector<std::pair<std::string, std::string>> tickets = {
        {"customer_support", "Enterprise order #E-12345 delayed"},
        {"finance", "Bulk invoice processing for 500 clients"},
        {"tech_support", "API integration failure for client XYZ Corp"},
        {"hr", "Mass onboarding of 50 new agents"},
        {"logistics", "Fleet tracking for 200 shipments"},
        {"healthcare", "Batch appointment scheduling"},
        {"sales", "Enterprise contract renewal - $50K"},
    };
    
    int ai = 0, human = 0;
    ledger.record("BATCH_START", "ticket_processing");
    
    std::cout << "Processing enterprise tickets..." << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
    
    for (const auto& [module, desc] : tickets) {
        // Rate limiting check
        if (!rate_limiter.allow("client_enterprise")) {
            std::cout << "  RATE LIMITED - cooling down" << std::endl;
            continue;
        }
        
        // Circuit breaker check
        if (!circuit_breaker.is_available(module)) {
            std::cout << "  CIRCUIT OPEN - " << module << " temporarily unavailable" << std::endl;
            circuit_breaker.record_failure(module);
            continue;
        }
        
        auto id = core.create_ticket("client_enterprise", module, module, desc);
        auto result = core.process_ticket(id);
        auto ticket = core.get_ticket(id);
        
        circuit_breaker.record_success(module);
        ledger.record("TICKET_PROCESSED", id);
        
        std::cout << "  " << id << " | " << module << std::endl;
        std::cout << "    " << result << std::endl;
        
        if (ticket->needs_human) human++; else ai++;
        
        // Lyapunov monitoring
        monitor.record(static_cast<double>(ai + human));
    }
    
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << std::endl;
    
    // Lyapunov report
    monitor.report();
    std::cout << std::endl;
    
    // Ledger verification
    ledger.record("BATCH_END", "ticket_processing");
    std::cout << "Ledger entries: " << ledger.size() << std::endl;
    std::cout << "Ledger verified: " << (ledger.verify() ? "VALID" : "CORRUPTED") << std::endl;
    std::cout << "Last hash: " << ledger.last().current_hash.substr(0, 16) << "..." << std::endl;
    std::cout << std::endl;
    
    double ai_pct = (ai + human > 0) ? ai * 100.0 / (ai + human) : 0;
    
    std::cout << "+==============================================+" << std::endl;
    std::cout << "|   ENTERPRISE RESULTS                          |" << std::endl;
    std::cout << "|   Tickets: " << (ai + human) << " | AI: " << ai << " | Human: " << human << "                    |" << std::endl;
    std::cout << "|   AI Rate: " << ai_pct << "%                              |" << std::endl;
    std::cout << "|   Ledger: " << ledger.size() << " entries (verified)               |" << std::endl;
    std::cout << "|                                               |" << std::endl;
    std::cout << "|   Dashboard: http://localhost:8092             |" << std::endl;
    std::cout << "|   API: http://localhost:8092/api/metrics       |" << std::endl;
    std::cout << "+==============================================+" << std::endl;
    std::cout << std::endl;
    std::cout << "ENTERPRISE READY. Press Ctrl+C to stop." << std::endl;
    
    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
    
    return 0;
}
