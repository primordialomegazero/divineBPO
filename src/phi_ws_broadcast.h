// PHI-WS-BROADCAST — Real-time WebSocket Pub/Sub
// Broadcast to all connected clients

#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
typedef int SOCKET;
#endif

namespace divine {

struct WSClient {
    SOCKET fd;
    std::string subscribed_room;
    bool alive;
};

class WSBroadcast {
private:
    std::vector<WSClient> clients;
    std::mutex mtx;
    
public:
    void add(SOCKET fd, const std::string& room = "global") {
        std::unique_lock<std::mutex> lock(mtx);
        clients.push_back({fd, room, true});
    }
    
    void remove(SOCKET fd) {
        std::unique_lock<std::mutex> lock(mtx);
        clients.erase(
            std::remove_if(clients.begin(), clients.end(), 
                [fd](const WSClient& c) { return c.fd == fd; }),
            clients.end());
    }
    
    void broadcast(const std::string& message, const std::string& room = "global") {
        std::unique_lock<std::mutex> lock(mtx);
        std::string frame = "\x81" + std::string(1, (char)message.size()) + message;
        for (auto& client : clients) {
            if (client.alive && (room == "global" || client.subscribed_room == room)) {
                send(client.fd, frame.c_str(), frame.size(), 0);
            }
        }
    }
    
    size_t count() { 
        std::unique_lock<std::mutex> lock(mtx); 
        return clients.size(); 
    }
};

} // namespace divine
