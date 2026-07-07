// PHI-ASYNC-HTTP v2 — Production-Grade Server
// Connection pool + Redis + Rate limit + WebSocket broadcast + API versioning

#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <vector>
#include <atomic>
#include <functional>
#include "phi_threadpool.h"
#include "phi_dbpool.h"
#include "phi_rate_limit.h"
#include "phi_ws_broadcast.h"

#ifdef __linux__
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define CLOSE_SOCKET close
#define MAX_EVENTS 10000
#endif

namespace http {

extern Metrics g_metrics;
extern std::vector<std::string> g_tickets_json;

class AsyncPhiServerV2 {
private:
    SOCKET server_fd;
    int epoll_fd;
    int port;
    std::atomic<bool> running;
    divine::DBPool* db_pool;
    divine::RedisCache* cache;
    divine::RateLimiter* limiter;
    divine::WSBroadcast* ws_broadcast;

    static void set_nonblocking(SOCKET fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    static std::string read_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::stringstream buf; buf << file.rdbuf(); return buf.str();
    }

    static std::string get_mime(const std::string& path) {
        if (path.find(".html") != std::string::npos) return "text/html";
        if (path.find(".css") != std::string::npos)  return "text/css";
        if (path.find(".js") != std::string::npos)   return "application/javascript";
        if (path.find(".json") != std::string::npos) return "application/json";
        return "text/plain";
    }

    static std::string ok_json(const std::string& j) {
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nX-API-Version: v23.1\r\nContent-Length: " + std::to_string(j.size()) + "\r\n\r\n" + j;
    }

    std::string handle_api(const std::string& path, const std::string& body) {
        char buf[4096];
        
        // API v1 routes
        if (path == "/api/v1/health" || path == "/api/health") {
            snprintf(buf, sizeof(buf), 
                "{\"status\":\"agi_enterprise\",\"version\":\"23.1\","
                "\"pqc\":\"%s\",\"concurrent\":%d,\"requests\":%d,"
                "\"db_pool\":%zu,\"cache\":\"%s\","
                "\"uptime\":%d}",
                g_metrics.pqc_algorithm.c_str(), 
                g_metrics.active_connections.load(), g_metrics.total_requests.load(),
                db_pool ? db_pool->size() : 0,
                (cache && cache->is_available()) ? "redis" : "none",
                g_metrics.uptime_seconds);
            return ok_json(buf);
        }

        if (path == "/api/v1/stats" || path == "/api/stats") {
            snprintf(buf, sizeof(buf),
                "{\"total_tickets\":%d,\"ai_rate\":%.1f,\"db_tickets\":%d,"
                "\"grc_blocks\":%d,\"redis_ok\":%s,\"rabbitmq_ok\":%s,"
                "\"pqc_secured\":%s,\"pqc_algorithm\":\"%s\",\"pqc_bits\":%d,"
                "\"active_connections\":%d,\"total_requests\":%d,"
                "\"ws_clients\":%zu,\"uptime\":%d}",
                g_metrics.total_tickets, g_metrics.ai_rate, g_metrics.db_tickets,
                g_metrics.grc_blocks,
                g_metrics.redis_ok ? "true" : "false", g_metrics.rabbitmq_ok ? "true" : "false",
                g_metrics.pqc_ok ? "true" : "false", g_metrics.pqc_algorithm.c_str(), g_metrics.pqc_security_bits,
                g_metrics.active_connections.load(), g_metrics.total_requests.load(),
                ws_broadcast ? ws_broadcast->count() : 0,
                g_metrics.uptime_seconds);
            return ok_json(buf);
        }

        if (path == "/api/v1/tickets" || path == "/api/tickets") {
            // Check cache first
            std::string cached;
            if (cache && cache->is_available()) {
                cached = cache->get("tickets_json");
            }
            if (!cached.empty()) return ok_json(cached);

            std::string json = "[";
            for (size_t i = 0; i < g_tickets_json.size(); i++) {
                if (i > 0) json += ",";
                json += g_tickets_json[i];
            }
            json += "]";
            
            // Cache for 30 seconds
            if (cache) cache->set("tickets_json", json, 30);
            return ok_json(json);
        }

        return "";
    }

    std::string handle_request(const std::string& request) {
        std::string path = "/index.html";
        size_t pos = request.find("GET /");
        if (pos != std::string::npos) {
            size_t end = request.find(" ", pos + 5);
            path = request.substr(pos + 4, end - pos - 4);
        }
        pos = request.find("POST /");
        if (pos != std::string::npos) {
            size_t end = request.find(" ", pos + 6);
            path = request.substr(pos + 5, end - pos - 5);
        }
        if (request.find("OPTIONS /") != std::string::npos) {
            return "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\nX-API-Version: v23.1\r\nContent-Length: 0\r\n\r\n";
        }
        if (path == "/") path = "/index.html";

        if (path.find("/api/") == 0) {
            std::string r = handle_api(path, "");
            if (!r.empty()) return r;
        }

        std::string content = read_file("public" + path);
        if (content.empty()) return "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: 9\r\n\r\n404 Not Found";

        std::string mime = get_mime(path);
        return "HTTP/1.1 200 OK\r\nContent-Type: " + mime + "\r\nAccess-Control-Allow-Origin: *\r\nX-API-Version: v23.1\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
    }

public:
    AsyncPhiServerV2(int p = 8092) : port(p), running(false), server_fd(INVALID_SOCKET), epoll_fd(-1),
                                       db_pool(nullptr), cache(nullptr), limiter(nullptr), ws_broadcast(nullptr) {}

    void set_db_pool(divine::DBPool* p) { db_pool = p; }
    void set_cache(divine::RedisCache* c) { cache = c; }
    void set_limiter(divine::RateLimiter* l) { limiter = l; }
    void set_ws_broadcast(divine::WSBroadcast* w) { ws_broadcast = w; }

    void start() {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == INVALID_SOCKET) return;

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        set_nonblocking(server_fd);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        bind(server_fd, (sockaddr*)&addr, sizeof(addr));
        listen(server_fd, SOMAXCONN);

        epoll_fd = epoll_create1(0);
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = server_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

        running = true;
        std::cout << "\n========================================" << std::endl;
        std::cout << "  DIVINE BPO v23.1 — PRODUCTION READY" << std::endl;
        std::cout << "  Port: " << port << " | 10K Concurrent" << std::endl;
        std::cout << "  DB Pool: " << (db_pool ? std::to_string(db_pool->size()) : "single") << std::endl;
        std::cout << "  Cache: " << ((cache && cache->is_available()) ? "Redis" : "None") << std::endl;
        std::cout << "  Rate Limit: 100 req/s/IP" << std::endl;
        std::cout << "  WebSocket: Broadcast ready" << std::endl;
        std::cout << "  API: /api/v1/*" << std::endl;
        std::cout << "========================================\n" << std::endl;

        epoll_event events[MAX_EVENTS];

        while (running) {
            int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);

            for (int i = 0; i < nfds; i++) {
                if (events[i].data.fd == server_fd) {
                    while (true) {
                        sockaddr_in ca{}; socklen_t cl = sizeof(ca);
                        SOCKET cf = accept(server_fd, (sockaddr*)&ca, &cl);
                        if (cf == INVALID_SOCKET) break;

                        // Rate limit check
                        char ip_str[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &ca.sin_addr, ip_str, sizeof(ip_str));
                        if (limiter && !limiter->allow(ip_str)) {
                            std::string rl = "HTTP/1.1 429 Too Many Requests\r\nRetry-After: 1\r\nContent-Length: 0\r\n\r\n";
                            send(cf, rl.c_str(), rl.size(), 0);
                            CLOSE_SOCKET(cf);
                            continue;
                        }

                        set_nonblocking(cf);
                        ev.events = EPOLLIN | EPOLLET;
                        ev.data.fd = cf;
                        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cf, &ev);
                        g_metrics.active_connections++;
                    }
                } else {
                    SOCKET cf = events[i].data.fd;
                    divine::g_pool->enqueue([cf, this]() {
                        char request_buf[16384] = {0};
                        ssize_t n = recv(cf, request_buf, sizeof(request_buf) - 1, 0);

                        if (n > 0) {
                            g_metrics.total_requests++;
                            std::string response = handle_request(std::string(request_buf));
                            send(cf, response.c_str(), response.size(), 0);
                        }

                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cf, nullptr);
                        CLOSE_SOCKET(cf);
                        g_metrics.active_connections--;
                    });
                }
            }
        }
    }

    void stop() { running = false; CLOSE_SOCKET(server_fd); if (epoll_fd >= 0) close(epoll_fd); }
};

} // namespace http
