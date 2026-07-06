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

std::atomic<bool> running{true};
void signal_handler(int) { running = false; }

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << R"(
+==============================================+
|  DIVINE BPO -- PRODUCTION v13.0               |
|  SQLite + Auth + Unique IDs                   |
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
    
    auto token = auth.login("admin");
    ledger.record("PROD_START", "v13_unique_ids");
    
    std::cout << "Systems: SQLite | Auth: " << (auth.validate(token) ? "OK" : "FAIL")
              << " | Swarm: " << swarm.total_cores() << " cores" << std::endl;
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
    int ai = 0, human = 0;
    
    for (const auto& [module, desc] : tickets) {
        if (!running) break;
        if (!rate_limiter.allow("prod")) continue;
        
        std::string response = swarm.process(module, desc);
        bool needs_human = (desc.find("URGENT") != std::string::npos);
        
        // Unique ID with microsecond timestamp
        auto us = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::string ticket_id = "TKT-" + std::to_string(us);
        auto now = std::time(nullptr);
        storage.insert_ticket(ticket_id, "prod_client", module, module, desc, needs_human ? "escalated" : "ai_resolved", needs_human, now, response.substr(0, 80));
        
        auto block = grc.mine_block(wallet);
        ledger.record("PROD_TXN", ticket_id);
        
        std::cout << "  [" << module << "] " << ticket_id.substr(0, 30) << "..."
                  << " | SQLite: saved | GRC #" << block.index << std::endl;
        
        if (needs_human) human++; else ai++;
        monitor.record(static_cast<double>(ai + human));
    }
    
    int ticket_count = storage.query_int("SELECT COUNT(*) FROM tickets");
    
    std::cout << std::endl;
    std::cout << "+==============================================+" << std::endl;
    std::cout << "|   v13.0 RESULTS                               |" << std::endl;
    std::cout << "|   Tickets: " << (ai + human) << " | AI: " << ai << " | Human: " << human << "                    |" << std::endl;
    std::cout << "|   SQLite: " << ticket_count << " tickets persisted                   |" << std::endl;
    std::cout << "|   GRC: " << grc.chain_length() << " blocks | " << (grc.verify_chain() ? "VALID" : "CORRUPT") << "                     |" << std::endl;
    std::cout << "|   Dashboard: http://localhost:8092             |" << std::endl;
    std::cout << "+==============================================+" << std::endl;
    std::cout << std::endl;
    std::cout << "Shutting down gracefully..." << std::endl;
    
    swarm.stop();
    leader.stop();
    ws.stop();
    
    std::cout << "[OK] Divine BPO stopped. All data persisted." << std::endl;
    return 0;
}
