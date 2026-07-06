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
#include "phi_config.h"
#include "phi_validate.h"

std::atomic<bool> running{true};
void signal_handler(int) { running = false; }

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << R"(
+==============================================+
|  DIVINE BPO -- v15.0 VALIDATED                |
|  Config + Input Validation + Error Handling   |
+==============================================+
)" << std::endl;
    
    divine::PhiStorage storage("divine_bpo.db");
    divine::PhiConfig config(storage);
    config.load_defaults();
    config.show();
    std::cout << std::endl;
    
    divine::PhiAuth auth(storage);
    divine::ErrorHandler errors;
    divine::Validator validator;
    divine::PhiSwarm swarm;
    divine::LyapunovMonitor monitor;
    divine::RateLimiter rate_limiter;
    divine::CircuitBreaker circuit_breaker;
    divine::WebSocketServer ws(config.get_int("ws_port", 8093));
    PhiLedger ledger;
    divine::GRCCoin grc;
    divine::PhiNetwork leader(config.get_int("mesh_port", 9000));
    
    http::PhiServer http_server(config.get_int("http_port", 8092));
    std::thread http_thread([&http_server]() { http_server.start(); });
    http_thread.detach();
    std::thread ws_thread([&ws]() { ws.start(); });
    ws_thread.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    leader.start(divine::LEADER);
    swarm.start(config.get_int("swarm_cores", 2));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto token = auth.login("admin");
    ledger.record("PROD_START", "v15_validated");
    
    std::vector<std::pair<std::string, std::string>> tickets = {
        {"customer_support", "Refund for damaged order"},
        {"tech_support", "Login issue after update"},
        {"healthcare", "URGENT: Surgery reschedule"},
        {"finance", "Duplicate charge investigation"},
        {"sales", "Enterprise plan inquiry"},
        {"logistics", "Shipment delayed by 5 days"},
        {"hr", "Onboarding documents needed"},
        {"ecommerce", "Return request for item"},
    };
    
    auto wallet = grc.create_wallet();
    int ai = 0, human = 0, failed = 0;
    
    for (const auto& [module, desc] : tickets) {
        if (!running) break;
        if (!rate_limiter.allow("prod")) continue;
        
        auto vt = validator.validate_ticket(module, desc);
        if (!vt.valid) {
            std::cout << "  [" << module << "] REJECTED: " << vt.error << std::endl;
            failed++;
            continue;
        }
        
        if (!circuit_breaker.is_available(vt.module)) {
            failed++;
            continue;
        }
        
        std::string response = errors.execute_with_fallback<std::string>(
            "ollama_ai",
            [&]() { return swarm.process(vt.module, vt.description); },
            [&]() { return "[FALLBACK] AI unavailable."; }
        );
        
        bool needs_human = (vt.description.find("URGENT") != std::string::npos) ||
                          (response.find("FALLBACK") != std::string::npos);
        
        auto us = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::string ticket_id = "TKT-" + std::to_string(us);
        auto now = std::time(nullptr);
        
        errors.execute("sqlite", [&]() {
            storage.insert_ticket(ticket_id, "prod_client", vt.module, vt.module,
                                 vt.description, needs_human ? "escalated" : "ai_resolved",
                                 needs_human, now, validator.truncate(response, 80));
            return true;
        });
        
        circuit_breaker.record_success(vt.module);
        auto block = grc.mine_block(wallet);
        ledger.record("PROD_TXN", ticket_id);
        
        std::cout << "  [" << vt.module << "] " << ticket_id.substr(0, 25) << "..."
                  << " | GRC #" << block.index;
        if (response.find("FALLBACK") != std::string::npos) std::cout << " | DEGRADED";
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
    std::cout << "|   v15.0 RESULTS                               |" << std::endl;
    std::cout << "|   Tickets: " << (ai + human) << " | AI: " << ai << " | Human: " << human << " | Failed: " << failed << "          |" << std::endl;
    std::cout << "|   SQLite: " << ticket_count << " persisted                       |" << std::endl;
    std::cout << "|   GRC: " << grc.chain_length() << " blocks | " << (grc.verify_chain() ? "VALID" : "CORRUPT") << "                     |" << std::endl;
    std::cout << "|   Dashboard: http://localhost:" << config.get("http_port") << "            |" << std::endl;
    std::cout << "+==============================================+" << std::endl;
    
    std::cout << "\nShutting down..." << std::endl;
    swarm.stop(); leader.stop(); ws.stop();
    std::cout << "[OK] Stopped." << std::endl;
    return 0;
}
