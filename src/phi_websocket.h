// PHI-WEBSOCKET — Real-time event system
// Broadcasts ticket updates to all connected clients
// Frame-based protocol on TCP

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>

namespace divine {

struct WSClient {
    int fd;
    std::string id;
    std::chrono::steady_clock::time_point connected_at;
};

class WebSocketServer {
private:
    int server_fd;
    int port;
    bool running;
    std::vector<WSClient> clients;
    std::mutex clients_mutex;
    
    std::string compute_accept_key(const std::string& key) {
        // Simplified WebSocket handshake accept
        std::string combined = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        // In production: SHA1 hash + base64 encode
        // For now: simple hash
        uint64_t h = 0;
        for (char c : combined) h = h * 31 + static_cast<uint64_t>(c);
        return std::to_string(h);
    }
    
    void send_frame(int client_fd, const std::string& message) {
        std::string frame;
        frame += static_cast<char>(0x81); // FIN + text opcode
        
        if (message.size() < 126) {
            frame += static_cast<char>(message.size());
        } else if (message.size() < 65536) {
            frame += static_cast<char>(126);
            frame += static_cast<char>((message.size() >> 8) & 0xFF);
            frame += static_cast<char>(message.size() & 0xFF);
        }
        
        frame += message;
        send(client_fd, frame.c_str(), frame.size(), 0);
    }
    
    bool handle_handshake(int client_fd, const std::string& request) {
        size_t key_pos = request.find("Sec-WebSocket-Key: ");
        if (key_pos == std::string::npos) return false;
        
        key_pos += 19;
        size_t key_end = request.find("\r\n", key_pos);
        std::string key = request.substr(key_pos, key_end - key_pos);
        
        std::string accept = compute_accept_key(key);
        
        std::string response = 
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
        
        send(client_fd, response.c_str(), response.size(), 0);
        return true;
    }

public:
    WebSocketServer(int p = 8093) : port(p), running(false) {}
    
    void start() {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) return;
        
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) return;
        if (listen(server_fd, 10) < 0) return;
        
        running = true;
        std::cout << "[WS] WebSocket server on port " << port << std::endl;
        
        while (running) {
            struct sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_fd < 0) continue;
            
            char buffer[4096] = {0};
            ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes > 0 && handle_handshake(client_fd, std::string(buffer))) {
                WSClient client;
                client.fd = client_fd;
                client.id = "ws-" + std::to_string(client_fd);
                client.connected_at = std::chrono::steady_clock::now();
                
                {
                    std::lock_guard<std::mutex> lock(clients_mutex);
                    clients.push_back(client);
                }
                
                // Welcome message
                send_frame(client_fd, R"({"type":"connected","message":"Divine BPO WebSocket active"})");
            }
        }
    }
    
    void broadcast(const std::string& event_type, const std::string& data) {
        std::string message = R"({"type":")" + event_type + R"(","data":)" + data + "}";
        
        std::lock_guard<std::mutex> lock(clients_mutex);
        
        auto it = clients.begin();
        while (it != clients.end()) {
            if (send(it->fd, nullptr, 0, 0) < 0) {
                close(it->fd);
                it = clients.erase(it);
            } else {
                send_frame(it->fd, message);
                ++it;
            }
        }
    }
    
    void broadcast_ticket_update(const std::string& ticket_id, 
                                  const std::string& status,
                                  const std::string& module) {
        std::string data = R"({"ticket_id":")" + ticket_id + 
                          R"(","status":")" + status + 
                          R"(","module":")" + module + R"("})";
        broadcast("ticket_update", data);
    }
    
    void stop() {
        running = false;
        close(server_fd);
    }
    
    size_t client_count() {
        std::lock_guard<std::mutex> lock(clients_mutex);
        return clients.size();
    }
};

} // namespace divine
