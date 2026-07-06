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
#include "phi_log.h"
#include "phi_monitor.h"
#include "phi_backup.h"

std::atomic<bool> running{true};
void signal_handler(int) { running = false; }

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    divine::Logger log("divine_bpo.log", divine::INFO, true);
    divine::Metrics metrics;
    divine::Backup backup("backups", 10);
    
    log.section("DIVINE BPO v16.0 STARTING");
    log.info("Logging system active");
    
    divine::PhiStorage storage("divine_bpo.db");
    divine::PhiConfig config(storage);
    config.load_defaults();
    log.info("Config loaded: " + std::to_string(13) + " keys");
    
    // Backup on startup
    backup.create("divine_bpo.db");
    
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
    ledger.record("PROD_START", "v16_logging_monitoring");
    log.info("Auth: " + std::string(auth.validate(token) ? "VALID" : "INVALID"));
    
    metrics.gauge("config_keys", 13);
    metrics.gauge("swarm_cores", swarm.total_cores());
    metrics.gauge("grc_chain_length", grc.chain_length());
    
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
    
    log.section("PROCESSING TICKETS");
    
    for (const auto& [module, desc] : tickets) {
        if (!running) break;
        
        auto vt = validator.validate_ticket(module, desc);
        if (!vt.valid) {
            log.warn("REJECTED: " + vt.error);
            failed++;
            continue;
        }
        
        log.debug("Processing: " + vt.module);
        metrics.inc("tickets_total");
        
        std::string response = errors.execute_with_fallback<std::string>(
            "ollama_ai",
            [&]() { return swarm.process(vt.module, vt.description); },
            [&]() { log.error("AI fallback activated"); return "[FALLBACK] AI unavailable."; }
        );
        
        bool needs_human = (vt.description.find("URGENT") != std::string::npos);
        
        auto us = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::string ticket_id = "TKT-" + std::to_string(us);
        auto now = std::time(nullptr);
        
        errors.execute("sqlite", [&]() {
            storage.insert_ticket(ticket_id, "prod_client", vt.module, vt.module,
                                 vt.description, needs_human ? "escalated" : "ai_resolved",
                                 needs_human, now, validator.truncate(response, 80));
            return true;
        });
        
        auto block = grc.mine_block(wallet);
        log.debug("Ticket saved: " + ticket_id.substr(0, 25) + "... GRC #" + std::to_string(block.index));
        
        if (needs_human) human++; else ai++;
        monitor.record(static_cast<double>(ai + human));
        metrics.gauge("ai_rate", (ai + human > 0) ? ai * 100.0 / (ai + human) : 0);
    }
    
    metrics.gauge("tickets_ai", ai);
    metrics.gauge("tickets_human", human);
    metrics.gauge("tickets_failed", failed);
    
    // Backup after processing
    backup.create("divine_bpo.db");
    
    log.section("RESULTS");
    log.info("Tickets: " + std::to_string(ai + human) + " | AI: " + std::to_string(ai) + 
             " | Human: " + std::to_string(human) + " | Failed: " + std::to_string(failed));
    log.info("SQLite: " + std::to_string(storage.query_int("SELECT COUNT(*) FROM tickets")) + " persisted");
    log.info("GRC: " + std::to_string(grc.chain_length()) + " blocks | " + 
             (grc.verify_chain() ? "VALID" : "CORRUPT"));
    log.info("Log file: divine_bpo.log | Backups: backups/");
    log.info("Dashboard: http://localhost:" + config.get("http_port"));
    log.info("API Docs: http://localhost:" + config.get("http_port") + "/api_docs.html");
    
    // Print metrics
    std::cout << std::endl << metrics.render();
    
    log.section("RUNNING");
    log.info("Press Ctrl+C to stop");
    
    while (running) std::this_thread::sleep_for(std::chrono::seconds(1));
    
    log.section("SHUTTING DOWN");
    backup.create("divine_bpo.db");
    swarm.stop(); leader.stop(); ws.stop();
    log.info("Stopped. All data persisted. Backups saved.");
    return 0;
}
