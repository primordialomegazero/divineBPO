// PHI-HTTP — Minimal HTTP Server for Divine BPO
// Serves static files + API endpoints

#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <thread>
#include <chrono>

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define CLOSE_SOCKET close
#else
#error "Linux only for now"
#endif

namespace http {

class PhiServer {
private:
    SOCKET server_fd;
    int port;
    bool running;
    
    static std::string read_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::stringstream buf;
        buf << file.rdbuf();
        return buf.str();
    }
    
    static std::string get_mime(const std::string& path) {
        if (path.find(".html") != std::string::npos) return "text/html";
        if (path.find(".css") != std::string::npos)  return "text/css";
        if (path.find(".js") != std::string::npos)   return "application/javascript";
        if (path.find(".json") != std::string::npos) return "application/json";
        return "text/plain";
    }
    
    std::string handle_request(const std::string& request) {
        std::string path = "/index.html";
        size_t pos = request.find("GET /");
        if (pos != std::string::npos) {
            size_t end = request.find(" ", pos + 5);
            path = request.substr(pos + 4, end - pos - 4);
            if (path == "/") path = "/index.html";
        }
        
        // API endpoints
        if (path == "/api/metrics") {
            return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
                   "{\"total_tickets\":10,\"ai_handled\":7,\"human_escalated\":3,"
                   "\"ai_rate\":70,\"modules_active\":14,\"lambda\":0.4812}";
        }
        
        if (path == "/api/info") {
            return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n"
                   "{\"name\":\"Divine BPO\",\"version\":\"1.0.0\","
                   "\"phi\":1.6180339887498948482,\"status\":\"operational\"}";
        }
        
        // Static files
        std::string file_path = "public" + path;
        std::string content = read_file(file_path);
        
        if (content.empty()) {
            return "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n"
                   "404 Not Found";
        }
        
        std::string mime = get_mime(path);
        return "HTTP/1.1 200 OK\r\nContent-Type: " + mime + "\r\n"
               "Access-Control-Allow-Origin: *\r\n"
               "Content-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
    }

public:
    PhiServer(int p = 8092) : port(p), running(false) {}
    
    void start() {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == INVALID_SOCKET) {
            std::cerr << "Failed to create socket" << std::endl;
            return;
        }
        
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            std::cerr << "Failed to bind port " << port << std::endl;
            return;
        }
        
        if (listen(server_fd, 10) == SOCKET_ERROR) {
            std::cerr << "Failed to listen" << std::endl;
            return;
        }
        
        running = true;
        std::cout << "\nServer running at http://localhost:" << port << std::endl;
        std::cout << "Press Ctrl+C to stop\n" << std::endl;
        
        while (running) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            SOCKET client = accept(server_fd, (sockaddr*)&client_addr, &client_len);
            
            if (client == INVALID_SOCKET) continue;
            
            char buffer[4096] = {0};
            ssize_t bytes = read(client, buffer, sizeof(buffer) - 1); (void)bytes;
            
            std::string response = handle_request(std::string(buffer));
            send(client, response.c_str(), response.size(), 0);
            CLOSE_SOCKET(client);
        }
    }
    
    void stop() {
        running = false;
        CLOSE_SOCKET(server_fd);
    }
};

} // namespace http
