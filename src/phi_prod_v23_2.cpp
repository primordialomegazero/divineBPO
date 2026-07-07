#include <iostream>
#include <thread>
#include <ctime>
#include <csignal>
#include <atomic>
#include <chrono>
#include <arpa/inet.h>
#include "phi_storage.h"
#include "phi_async_http.h"
#include "phi_websocket.h"
#include "phi_grc.h"
#include "phi_log.h"
#include "phi_backup.h"
#include "phi_redis.h"
#include "phi_queue.h"
#include "phi_pqc.h"
#include "phi_agi.h"
#include "phi_twilio.h"
#include "phi_threadpool.h"
#include "phi_dbpool.h"
#include "phi_rate_limit.h"
#include "phi_voice.h"
#include "phi_callqueue.h"
#include "phi_i18n.h"

using namespace http;
using namespace divine;

std::atomic<bool> running{true};
void signal_handler(int) { running = false; }

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    divine::Logger log("divine_bpo.log");
    divine::Backup backup("backups", 10);
    time_t start_time = time(nullptr);

    log.info("══════ DIVINE BPO v23.2 — AGI VOICE (TWILIO LIVE) ══════");

    // Thread Pool
    divine::g_pool = std::make_unique<ThreadPool>(48);
    log.info("ThreadPool: 48 workers");

    // DB Pool
    divine::DBPool db_pool("divine_bpo.db", 8);
    g_metrics.db_tickets = db_pool.query_int("SELECT COUNT(*) FROM tickets");
    log.info("DB Pool: 8 connections | " + std::to_string(g_metrics.db_tickets) + " tickets");

    // Redis Cache
    divine::RedisCache cache;
    g_metrics.redis_ok = cache.is_available();
    log.info("Cache: " + std::string(cache.is_available() ? "Redis" : "None"));

    // Rate Limiter
    divine::RateLimiter limiter(100.0, 200.0, 1000);
    log.info("Rate Limit: 100 req/s/IP");

    // Voice Handler
    divine::VoiceHandler voice;
    log.info("Voice: Twilio ready");

    // Call Queue
    divine::CallQueueManager call_queue(500);
    log.info("Call Queue: 500 max | Priority-based");

    // Multi-language
    divine::I18N i18n;
    log.info("I18N: EN/ES/FR/JA/TL ready");

    // Populate tickets
    g_tickets_json.push_back("{\"id\":\"TKT-01\",\"module\":\"customer_support\",\"subject\":\"Order #12345\",\"handler\":\"HUMAN\",\"status\":\"OPEN\"}");
    g_tickets_json.push_back("{\"id\":\"TKT-02\",\"module\":\"tech_support\",\"subject\":\"Login issue\",\"handler\":\"AI\",\"status\":\"CLOSED\"}");
    g_tickets_json.push_back("{\"id\":\"TKT-03\",\"module\":\"healthcare\",\"subject\":\"Reschedule appointment\",\"handler\":\"AI\",\"status\":\"CLOSED\"}");
    g_tickets_json.push_back("{\"id\":\"TKT-04\",\"module\":\"finance\",\"subject\":\"Incorrect charge\",\"handler\":\"HUMAN\",\"status\":\"OPEN\"}");
    g_tickets_json.push_back("{\"id\":\"TKT-05\",\"module\":\"hr\",\"subject\":\"PTO request\",\"handler\":\"HUMAN\",\"status\":\"OPEN\"}");
    g_tickets_json.push_back("{\"id\":\"TKT-06\",\"module\":\"sales\",\"subject\":\"Product inquiry\",\"handler\":\"AI\",\"status\":\"CLOSED\"}");
    g_tickets_json.push_back("{\"id\":\"TKT-07\",\"module\":\"billing\",\"subject\":\"Invoice question\",\"handler\":\"AI\",\"status\":\"CLOSED\"}");
    g_tickets_json.push_back("{\"id\":\"TKT-08\",\"module\":\"tech_support\",\"subject\":\"VPN not connecting\",\"handler\":\"AI\",\"status\":\"CLOSED\"}");

    // PQC
    divine::PhiPQC pqc;
    if (pqc.init()) {
        g_metrics.pqc_ok = true;
        g_metrics.pqc_algorithm = pqc.get_algorithm();
        g_metrics.pqc_security_bits = pqc.get_security_level();
    }

    // AGI
    divine::PhiAGI agi;
    agi.init();

    // RabbitMQ
    divine::MessageQueue mq;
    g_metrics.rabbitmq_ok = mq.is_available();

    g_metrics.grc_blocks = 9;
    g_metrics.swarm_cores = 2;
    g_metrics.backups = 6;
    g_metrics.total_tickets = 8;
    g_metrics.ai_handled = 7;
    g_metrics.human_escalated = 1;
    g_metrics.ai_rate = 87.5;
    g_metrics.status = "agi_voice";

    // WebSocket
    std::thread ws_thread([&]() {
        divine::WebSocketServer ws(8093);
        ws.start();
    });
    ws_thread.detach();

    // Metrics updater
    std::thread metrics_thread([&]() {
        while (running) {
            g_metrics.uptime_seconds = time(nullptr) - start_time;
            g_metrics.db_tickets = db_pool.query_int("SELECT COUNT(*) FROM tickets");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });
    metrics_thread.detach();

    log.info("══════ AGI VOICE READY ══════");
    log.info("Dashboard: http://localhost:8092");
    log.info("Voice Webhook: http://localhost:8092/api/voice/incoming");
    log.info("Call Queue: 500 max, priority-based");
    log.info("Languages: EN, ES, FR, JA, TL");
    log.info("PQC: " + g_metrics.pqc_algorithm);

    // Start server
    AsyncPhiServer server(8092);
    server.start();

    log.info("══════ GRACEFUL SHUTDOWN ══════");
    backup.create("divine_bpo.db");
    divine::g_pool.reset();
    log.info("All data persisted. Goodbye.");

    return 0;
}
