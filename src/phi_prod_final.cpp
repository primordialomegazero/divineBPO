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
#include "phi_notify.h"
#include "phi_tls.h"

std::atomic<bool> running{true};
void signal_handler(int) { running = false; }

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    divine::Logger log("divine_bpo.log");
    divine::Metrics metrics;
    divine::Backup backup("backups", 10);
    divine::Notifier notify;
    
    log.section("DIVINE BPO v17.0 — PRODUCTION FINAL");
    
    divine::PhiStorage storage("divine_bpo.db");
    divine::PhiConfig config(storage);
    config.load_defaults();
    
    backup.create("divine_bpo.db");
    metrics.gauge("uptime_seconds", 0);
    
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
    
    // TLS setup
    if (divine::TLSConfig::check_openssl()) {
        divine::TLSConfig::generate_self_signed();
    }
    
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
    ledger.record("PROD_FINAL", "v17_all_systems");
    
    log.info("Systems: SQLite | Auth | Swarm | GRC | Network | TLS | Backup | Notify");
    log.info("Dashboard: http://localhost:" + config.get("http_port"));
    log.info("API Docs: http://localhost:" + config.get("http_port") + "/api_docs.html");
    
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
    
    log.section("PROCESSING " + std::to_string(tickets.size()) + " TICKETS");
    
    for (const auto& [module, desc] : tickets) {
        if (!running) break;
        
        auto vt = validator.validate_ticket(module, desc);
        if (!vt.valid) { failed++; continue; }
        
        std::string response = errors.execute_with_fallback<std::string>(
            "ollama_ai",
            [&]() { return swarm.process(vt.module, vt.description); },
            [&]() { notify.alert_failure("ollama_ai", "AI unavailable"); 
                   return "[FALLBACK] AI unavailable."; }
        );
        
        bool needs_human = (vt.description.find("URGENT") != std::string::npos);
        metrics.inc("tickets_total");
        
        auto us = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::string ticket_id = "TKT-" + std::to_string(us);
        auto now = std::time(nullptr);
        
        storage.insert_ticket(ticket_id, "prod_client", vt.module, vt.module,
                             vt.description, needs_human ? "escalated" : "ai_resolved",
                             needs_human, now, validator.truncate(response, 80));
        
        if (needs_human) {
            notify.alert_escalation(ticket_id, vt.module, vt.description);
            human++;
        } else {
            ai++;
        }
        
        auto block = grc.mine_block(wallet);
        circuit_breaker.record_success(vt.module);
        monitor.record(static_cast<double>(ai + human));
        metrics.gauge("ai_rate", (ai + human > 0) ? ai * 100.0 / (ai + human) : 0);
    }
    
    metrics.gauge("tickets_ai", ai);
    metrics.gauge("tickets_human", human);
    metrics.gauge("grc_blocks", grc.chain_length());
    metrics.gauge("db_tickets", storage.query_int("SELECT COUNT(*) FROM tickets"));
    
    backup.create("divine_bpo.db");
    notify.daily_report(ai + human, ai, human, failed,
                        (ai + human > 0) ? ai * 100.0 / (ai + human) : 0,
                        grc.chain_length(), grc.verify_chain());
    
    log.section("RESULTS");
    log.info("Tickets: " + std::to_string(ai + human) + " | AI: " + std::to_string(ai) + 
             " | Human: " + std::to_string(human));
    log.info("All systems operational. Press Ctrl+C to stop.");
    
    std::cout << std::endl << metrics.render();
    
    while (running) std::this_thread::sleep_for(std::chrono::seconds(1));
    
    log.section("SHUTDOWN");
    backup.create("divine_bpo.db");
    swarm.stop(); leader.stop(); ws.stop();
    log.info("Stopped. Data persisted. Backups saved.");
    return 0;
}
