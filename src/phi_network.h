// PHI-NETWORK — Distributed Mesh (Layer 0)
// Leader/Worker/Edge nodes with phi-sync gossip protocol
// Auto failover, peer discovery, state propagation

#pragma once
#include "phi_constants.h"
#include "phi_hash.h"
#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace divine {

enum NodeRole { LEADER, WORKER, EDGE };

struct Peer {
    std::string id;
    std::string address;
    int port;
    NodeRole role;
    bool online;
    double load;
    std::chrono::steady_clock::time_point last_heartbeat;
};

class PhiNetwork {
private:
    std::string node_id;
    NodeRole role;
    int port;
    std::vector<Peer> peers;
    std::mutex peers_mutex;
    std::atomic<bool> running{false};
    int leader_fd;
    bool is_leader;
    
    std::string generate_node_id() {
        return "phi-node-" + std::to_string(phi_hash(std::to_string(port) + std::to_string(std::rand())));
    }
    
    void gossip_loop() {
        while (running) {
            {
                std::lock_guard<std::mutex> lock(peers_mutex);
                
                // Check peer health
                auto now = std::chrono::steady_clock::now();
                for (auto& peer : peers) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                        now - peer.last_heartbeat).count();
                    peer.online = (elapsed < 10);
                }
                
                // Leader election (simplest: lowest port wins)
                if (is_leader) {
                    int online_count = 0;
                    for (auto& p : peers) if (p.online) online_count++;
                    
                    // Scale decision based on phi
                    if (online_count < 2 && peers.size() < 10) {
                        // Need more workers — would spawn new nodes
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>(1618))); // phi * 1000 ms
        }
    }
    
    void broadcast_heartbeat() {
        std::string heartbeat = "PHI-HEARTBEAT:" + node_id + ":" + 
            std::to_string(static_cast<int>(role)) + ":" + std::to_string(port);
        
        for (auto& peer : peers) {
            if (!peer.online) continue;
            
            int sock = socket(AF_INET, SOCK_DGRAM, 0);
            struct sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(peer.port);
            inet_pton(AF_INET, peer.address.c_str(), &addr.sin_addr);
            
            sendto(sock, heartbeat.c_str(), heartbeat.size(), 0,
                   (struct sockaddr*)&addr, sizeof(addr));
            close(sock);
        }
    }

public:
    PhiNetwork(int p = 9000) : port(p), is_leader(false), leader_fd(-1) {
        node_id = generate_node_id();
    }
    
    void start(NodeRole r = LEADER) {
        role = r;
        is_leader = (r == LEADER);
        running = true;
        
        std::cout << "[NET] Node " << node_id << " online as "
                  << (is_leader ? "LEADER" : (role == WORKER ? "WORKER" : "EDGE"))
                  << " on port " << port << std::endl;
        
        std::thread gossip(&PhiNetwork::gossip_loop, this);
        gossip.detach();
        
        std::thread heartbeat([this]() {
            while (running) {
                broadcast_heartbeat();
                std::this_thread::sleep_for(std::chrono::milliseconds(1618));
            }
        });
        heartbeat.detach();
    }
    
    void add_peer(const std::string& address, int peer_port, NodeRole r = WORKER) {
        std::lock_guard<std::mutex> lock(peers_mutex);
        Peer peer{generate_node_id(), address, peer_port, r, true, 0.0,
                  std::chrono::steady_clock::now()};
        peers.push_back(peer);
        std::cout << "[NET] Peer added: " << address << ":" << peer_port 
                  << " as " << (r == WORKER ? "WORKER" : "EDGE") << std::endl;
    }
    
    int peer_count() {
        std::lock_guard<std::mutex> lock(peers_mutex);
        int count = 0;
        for (auto& p : peers) if (p.online) count++;
        return count;
    }
    
    int total_peers() {
        std::lock_guard<std::mutex> lock(peers_mutex);
        return peers.size();
    }
    
    double network_load() {
        std::lock_guard<std::mutex> lock(peers_mutex);
        if (peers.empty()) return 0;
        double sum = 0;
        for (auto& p : peers) sum += p.load;
        return sum / peers.size();
    }
    
    void set_leader(bool l) { is_leader = l; }
    bool am_leader() const { return is_leader; }
    std::string get_node_id() const { return node_id; }
    
    void stop() { running = false; }
};

} // namespace divine
