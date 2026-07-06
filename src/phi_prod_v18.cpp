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
#include "phi_redis.h"
#include "phi_queue.h"

std::atomic<bool> running{true};
void signal_handler(int) { running = false; }

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    divine::Logger log("divine_bpo.log");
    divine::Metrics metrics;
    divine::Backup backup("backups", 10);
    divine::Notifier notify;
    divine::RedisDB redis;
    divine::MessageQueue mq;
    
    log.section("DIVINE BPO v18.0 — FULL INFRASTRUCTURE");
    
    divine::PhiStorage storage("divine_bpo.db");
    divine::PhiConfig config(storage);
    config.load_defaults();
    
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
    
    divine::TLSConfig::generate_self_signed();
    
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
    ledger.record("PROD_V18", "full_infra");
    
    // Infrastructure status
    log.info("Redis: " + std::string(redis.is_available() ? "AVAILABLE" : "UNAVAILABLE (using fallback)"));
    log.info("RabbitMQ: " + std::string(mq.is_available() ? "AVAILABLE" : "UNAVAILABLE (using fallback)"));
    log.info("NGINX: Configured in nginx_divine.conf");
    log.info("Prometheus: Configured in prometheus.yml");
    log.info("Grafana: Install and add Prometheus datasource on :3000");
    
    // Cache warm-up
    redis.set("app:version", "18.0");
    redis.set("app:status", "operational");
    redis.expire("app:status", 60);
    
    // Queue setup
    mq.declare_queue("tickets");
    mq.declare_dead_letter("tickets", "tickets_dlq");
    
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
    int ai = 0, human = 0;
    
    log.section("PROCESSING TICKETS (with Redis + MQ)");
    
    for (const auto& [module, desc] : tickets) {
        if (!running) break;
        
        auto vt = validator.validate_ticket(module, desc);
        if (!vt.valid) continue;
        
        // Publish to queue
        mq.publish("tickets", vt.module + ":" + vt.description);
        
        // Check cache
        std::string cached = redis.get("ticket:" + vt.module);
        
        std::string response = errors.execute_with_fallback<std::string>(
            "ollama_ai",
            [&]() { return swarm.process(vt.module, vt.description); },
            [&]() { return "[FALLBACK]"; }
        );
        
        // Cache result
        redis.set("ticket:" + vt.module, response.substr(0, 80));
        redis.expire("ticket:" + vt.module, 3600);
        redis.incr("stats:tickets_processed");
        
        bool needs_human = (vt.description.find("URGENT") != std::string::npos);
        
        auto us = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        storage.insert_ticket("TKT-" + std::to_string(us), "prod_client", vt.module, vt.module,
                             vt.description, needs_human ? "escalated" : "ai_resolved",
                             needs_human, std::time(nullptr), validator.truncate(response, 80));
        
        if (needs_human) { human++; notify.alert_escalation("TKT-" + std::to_string(us), vt.module, vt.description); }
        else ai++;
        
        grc.mine_block(wallet);
        metrics.inc("tickets_total");
        metrics.gauge("ai_rate", (ai + human > 0) ? ai * 100.0 / (ai + human) : 0);
    }
    
    metrics.gauge("db_tickets", storage.query_int("SELECT COUNT(*) FROM tickets"));
    metrics.gauge("redis_available", redis.is_available() ? 1 : 0);
    metrics.gauge("mq_available", mq.is_available() ? 1 : 0);
    
    backup.create("divine_bpo.db");
    
    log.section("INFRASTRUCTURE STATUS");
    log.info("SQLite: " + std::to_string(storage.query_int("SELECT COUNT(*) FROM tickets")) + " tickets");
    log.info("Redis: " + std::string(redis.is_available() ? "CONNECTED" : "FALLBACK"));
    log.info("RabbitMQ: " + std::string(mq.is_available() ? "CONNECTED" : "FALLBACK"));
    log.info("Prometheus: http://localhost:8092/metrics");
    log.info("Grafana: http://localhost:3000 (admin/admin)");
    log.info("NGINX: sudo ln -s nginx_divine.conf /etc/nginx/sites-enabled/");
    
    std::cout << std::endl << metrics.render();
    
    log.section("RUNNING");
    while (running) std::this_thread::sleep_for(std::chrono::seconds(1));
    
    log.section("SHUTDOWN");
    backup.create("divine_bpo.db");
    swarm.stop(); leader.stop(); ws.stop();
    log.info("Stopped. All systems saved.");
    return 0;
}
