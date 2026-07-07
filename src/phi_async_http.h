// PHI-ASYNC-HTTP — Non-blocking HTTP Server
// Thread pool + epoll for 10k concurrent

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

struct Metrics {
    int total_tickets = 0, ai_handled = 0, human_escalated = 0;
    double ai_rate = 0.0;
    int modules_active = 14;
    double lambda = 0.4812, phi = 1.6180339887498948482;
    int db_tickets = 0, grc_blocks = 0, backups = 0;
    bool redis_ok = false, rabbitmq_ok = false;
    int swarm_cores = 0, uptime_seconds = 0;
    bool pqc_ok = false;
    std::string pqc_algorithm = "None";
    int pqc_security_bits = 0;
    std::string status = "operational";
    std::atomic<int> active_connections{0};
    std::atomic<int> total_requests{0};
};

extern Metrics g_metrics;
extern std::vector<std::string> g_tickets_json;

class AsyncPhiServer {
private:
    SOCKET server_fd;
    int epoll_fd;
    int port;
    std::atomic<bool> running;

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
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + std::to_string(j.size()) + "\r\n\r\n" + j;
    }

    static std::string handle_api(const std::string& path, const std::string& body) {
        char buf[4096];
        
        if (path == "/api/health") {
            snprintf(buf, sizeof(buf), "{\"status\":\"%s\",\"pqc\":\"%s\",\"concurrent\":%d,\"requests\":%d,\"uptime\":%d,\"version\":\"23.0\"}", 
                     g_metrics.status.c_str(), g_metrics.pqc_algorithm.c_str(), 
                     g_metrics.active_connections.load(), g_metrics.total_requests.load(),
                     g_metrics.uptime_seconds);
            return ok_json(buf);
        }

        if (path == "/api/stats") {
            snprintf(buf, sizeof(buf),
                "{\"total_tickets\":%d,\"ai_handled\":%d,\"human_escalated\":%d,\"ai_rate\":%.1f,\"modules_active\":%d,\"lambda\":%.4f,\"phi\":%.4f,\"db_tickets\":%d,\"grc_blocks\":%d,\"backups\":%d,\"redis_ok\":%s,\"rabbitmq_ok\":%s,\"swarm_cores\":%d,\"pqc_secured\":%s,\"pqc_algorithm\":\"%s\",\"pqc_bits\":%d,\"active_connections\":%d,\"total_requests\":%d,\"uptime\":%d}",
                g_metrics.total_tickets, g_metrics.ai_handled, g_metrics.human_escalated,
                g_metrics.ai_rate, g_metrics.modules_active, g_metrics.lambda, g_metrics.phi,
                g_metrics.db_tickets, g_metrics.grc_blocks, g_metrics.backups,
                g_metrics.redis_ok ? "true" : "false", g_metrics.rabbitmq_ok ? "true" : "false",
                g_metrics.swarm_cores,
                g_metrics.pqc_ok ? "true" : "false", g_metrics.pqc_algorithm.c_str(), g_metrics.pqc_security_bits,
                g_metrics.active_connections.load(), g_metrics.total_requests.load(),
                g_metrics.uptime_seconds);
            return ok_json(buf);
        }

        if (path == "/api/infra") {
            snprintf(buf, sizeof(buf),
                "{\"redis\":%s,\"rabbitmq\":%s,\"swarm_cores\":%d,\"grc_blocks\":%d,\"backups\":%d,\"pqc\":\"%s\",\"pool_workers\":%zu,\"uptime\":%d}",
                g_metrics.redis_ok ? "true" : "false", g_metrics.rabbitmq_ok ? "true" : "false",
                g_metrics.swarm_cores, g_metrics.grc_blocks, g_metrics.backups,
                g_metrics.pqc_algorithm.c_str(), divine::g_pool ? divine::g_pool->size() : 0,
                g_metrics.uptime_seconds);
            return ok_json(buf);
        }

        if (path == "/api/tickets") {
            std::string json = "[";
            for (size_t i = 0; i < g_tickets_json.size(); i++) {
                if (i > 0) json += ",";
                json += g_tickets_json[i];
            }
            json += "]";
            return ok_json(json);
        }

        return "";
    }

    static std::string handle_request(const std::string& request) {
        std::string path = "/index.html";
        std::string body;
        size_t pos = request.find("GET /");
        if (pos != std::string::npos) {
            size_t end = request.find(" ", pos + 5);
            path = request.substr(pos + 4, end - pos - 4);
        }
        pos = request.find("POST /");
        if (pos != std::string::npos) {
            size_t end = request.find(" ", pos + 6);
            path = request.substr(pos + 5, end - pos - 5);
            size_t body_pos = request.find("\r\n\r\n");
            if (body_pos != std::string::npos) body = request.substr(body_pos + 4);
        }
        if (request.find("OPTIONS /") != std::string::npos) {
            return "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\nContent-Length: 0\r\n\r\n";
        }
        if (path == "/") path = "/index.html";

        if (path.find("/api/") == 0) {
            std::string r = handle_api(path, body);
            if (!r.empty()) return r;
        }

        std::string content = read_file("public" + path);
        if (content.empty()) return "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: 9\r\n\r\n404 Not Found";

        std::string mime = get_mime(path);
        return "HTTP/1.1 200 OK\r\nContent-Type: " + mime + "\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
    }

public:
    AsyncPhiServer(int p = 8092) : port(p), running(false), server_fd(INVALID_SOCKET), epoll_fd(-1) {}

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

        if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Failed to bind port " << port << std::endl;
            return;
        }

        listen(server_fd, SOMAXCONN);

        // Create epoll
        epoll_fd = epoll_create1(0);
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = server_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

        running = true;
        std::cout << "\nServer running at http://localhost:" << port << " (10k concurrent ready)" << std::endl;

        epoll_event events[MAX_EVENTS];

        while (running) {
            int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);

            for (int i = 0; i < nfds; i++) {
                if (events[i].data.fd == server_fd) {
                    // Accept new connection
                    while (true) {
                        sockaddr_in ca{};
                        socklen_t cl = sizeof(ca);
                        SOCKET cf = accept(server_fd, (sockaddr*)&ca, &cl);
                        if (cf == INVALID_SOCKET) break;

                        set_nonblocking(cf);
                        ev.events = EPOLLIN | EPOLLET;
                        ev.data.fd = cf;
                        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cf, &ev);
                        g_metrics.active_connections++;
                    }
                } else {
                    // Handle client request
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

// Global instances
Metrics g_metrics;
std::vector<std::string> g_tickets_json;
// g_pool defined in phi_threadpool.h (divine namespace)

} // namespace http
