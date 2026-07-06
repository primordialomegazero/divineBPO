#include <iostream>
#include <thread>
#include <ctime>
#include <csignal>
#include <atomic>
#include <chrono>
#include "phi_storage.h"
#include "phi_auth.h"
#include "phi_http.h"
#include "phi_lyapunov.h"
#include "phi_rate_limiter.h"
#include "phi_circuit_breaker.h"
#include "phi_ledger.h"
#include "phi_swarm.h"
#include "phi_websocket.h"
#include "phi_grc.h"
#include "phi_network.h"
#include "phi_error.h"

std::atomic<bool> running{true};
void signal_handler(int) { running = false; }

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    divine::ErrorHandler errors;
    
    std::cout << R"(
+==============================================+
|  DIVINE BPO -- v14.0 ERROR-HARDENED           |
|  Retry Logic + Graceful Degradation           |
+==============================================+
)" << std::endl;
    
    divine::PhiStorage storage("divine_bpo.db");
    divine::PhiAuth auth(storage);
    divine::PhiSwarm swarm;
    divine::LyapunovMonitor monitor;
    divine::RateLimiter rate_limiter;
    divine::CircuitBreaker circuit_breaker;
    divine::WebSocketServer ws(8093);
    PhiLedger ledger;
    divine::GRCCoin grc;
    divine::PhiNetwork leader(9000);
    
    http::PhiServer http_server(8092);
    std::thread http_thread([&http_server]() { http_server.start(); });
    http_thread.detach();
    std::thread ws_thread([&ws]() { ws.start(); });
    ws_thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    leader.start(divine::LEADER);
    swarm.start(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Auth with retry
    std::string token;
    auto auth_result = errors.execute("auth", [&]() {
        token = auth.login("admin");
        return auth.validate(token);
    });
    
    if (auth_result == divine::ErrorHandler::OK) {
        ledger.record("PROD_START", "v14_error_handling");
        std::cout << "[OK] Auth: VALID" << std::endl;
    } else {
        std::cout << "[FAIL] Auth degraded — continuing without auth" << std::endl;
    }
    
    std::cout << std::endl;
    
    std::vector<std::pair<std::string, std::string>> tickets = {
        {"customer_support", "Refund for damaged order #12345"},
        {"tech_support", "Login issue after v2.0 update"},
        {"healthcare", "URGENT: Surgery reschedule needed"},
        {"finance", "Duplicate charge investigation"},
        {"sales", "Enterprise plan for 500 seats"},
        {"logistics", "Shipment delayed by 5 days"},
        {"hr", "Onboarding documents for new hire"},
        {"ecommerce", "Return request for item #9876"},
    };
    
    auto wallet = grc.create_wallet();
    int ai = 0, human = 0, failed = 0;
    
    for (const auto& [module, desc] : tickets) {
        if (!running) break;
        if (!rate_limiter.allow("prod")) continue;
        if (!circuit_breaker.is_available(module)) {
            std::cout << "  [" << module << "] CIRCUIT OPEN — skipped" << std::endl;
            failed++;
            continue;
        }
        
        // AI with retry and fallback
        std::string response = errors.execute_with_fallback<std::string>(
            "ollama_ai",
            [&]() { return swarm.process(module, desc); },
            [&]() { return "[FALLBACK] AI unavailable. Queued for human review."; }
        );
        
        bool needs_human = (desc.find("URGENT") != std::string::npos) ||
                          (response.find("FALLBACK") != std::string::npos);
        
        // DB insert with retry
        auto us = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::string ticket_id = "TKT-" + std::to_string(us);
        auto now = std::time(nullptr);
        
        auto db_result = errors.execute("sqlite", [&]() {
            storage.insert_ticket(ticket_id, "prod_client", module, module, desc,
                                 needs_human ? "escalated" : "ai_resolved",
                                 needs_human, now, response.substr(0, 80));
            return true;
        });
        
        if (db_result != divine::ErrorHandler::OK) {
            std::cerr << "  [" << module << "] DB WRITE FAILED after retries" << std::endl;
            failed++;
            continue;
        }
        
        circuit_breaker.record_success(module);
        auto block = grc.mine_block(wallet);
        ledger.record("PROD_TXN", ticket_id);
        
        std::cout << "  [" << module << "] " << ticket_id.substr(0, 30) << "..."
                  << " | DB: saved | GRC #" << block.index;
        if (response.find("FALLBACK") != std::string::npos) {
            std::cout << " | DEGRADED";
        }
        std::cout << std::endl;
        
        if (needs_human) human++; else ai++;
        monitor.record(static_cast<double>(ai + human));
    }
    
    int ticket_count = storage.query_int("SELECT COUNT(*) FROM tickets");
    
    std::cout << std::endl;
    errors.health_report();
    monitor.report();
    std::cout << std::endl;
    
    std::cout << "+==============================================+" << std::endl;
    std::cout << "|   v14.0 RESULTS                               |" << std::endl;
    std::cout << "|   Tickets: " << (ai + human) << " | AI: " << ai << " | Human: " << human << " | Failed: " << failed << "          |" << std::endl;
    std::cout << "|   SQLite: " << ticket_count << " persisted                       |" << std::endl;
    std::cout << "|   GRC: " << grc.chain_length() << " blocks | " << (grc.verify_chain() ? "VALID" : "CORRUPT") << "                     |" << std::endl;
    std::cout << "|   Dashboard: http://localhost:8092             |" << std::endl;
    std::cout << "+==============================================+" << std::endl;
    
    std::cout << "\nShutting down..." << std::endl;
    swarm.stop(); leader.stop(); ws.stop();
    std::cout << "[OK] Stopped. Data persisted." << std::endl;
    return 0;
}
