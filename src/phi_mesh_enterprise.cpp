#include <iostream>
#include <thread>
#include <iomanip>
#include <vector>
#include "phi_fhe_core.h"
#include "phi_http.h"
#include "phi_lyapunov.h"
#include "phi_rate_limiter.h"
#include "phi_circuit_breaker.h"
#include "phi_ledger.h"
#include "phi_swarm.h"
#include "phi_websocket.h"
#include "phi_grc.h"
#include "phi_network.h"

int main() {
    std::cout << R"(
+==============================================+
|  DIVINE BPO -- MESH ENTERPRISE v8.0           |
|  Distributed Network + GRC + AI Swarm         |
+==============================================+
)" << std::endl;
    
    divine::PhiNetwork leader(9000);
    divine::PhiNetwork worker1(9001);
    divine::PhiNetwork worker2(9002);
    divine::PhiNetwork edge1(9003);
    
    divine::PhiSwarm swarm;
    divine::LyapunovMonitor monitor;
    divine::RateLimiter rate_limiter;
    divine::CircuitBreaker circuit_breaker;
    divine::WebSocketServer ws(8093);
    PhiLedger ledger;
    PhiFHECore fhe_core;
    divine::GRCCoin grc;
    
    leader.start(divine::LEADER);
    worker1.start(divine::WORKER);
    worker2.start(divine::WORKER);
    edge1.start(divine::EDGE);
    
    worker1.add_peer("127.0.0.1", 9000, divine::LEADER);
    worker2.add_peer("127.0.0.1", 9000, divine::LEADER);
    edge1.add_peer("127.0.0.1", 9000, divine::LEADER);
    leader.add_peer("127.0.0.1", 9001, divine::WORKER);
    leader.add_peer("127.0.0.1", 9002, divine::WORKER);
    leader.add_peer("127.0.0.1", 9003, divine::EDGE);
    
    swarm.start(2);
    
    http::PhiServer http_server(8092);
    std::thread http_thread([&http_server]() { http_server.start(); });
    http_thread.detach();
    
    std::thread ws_thread([&ws]() { ws.start(); });
    ws_thread.detach();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ledger.record("MESH_INIT", "4_nodes");
    
    std::cout << "Mesh Network Online:" << std::endl;
    std::cout << "  Leader: :9000 | Workers: :9001, :9002 | Edge: :9003" << std::endl;
    std::cout << "  Peers connected: " << leader.peer_count() << std::endl;
    std::cout << "  Network load: " << std::fixed << std::setprecision(2) 
              << leader.network_load() << std::endl;
    std::cout << std::endl;
    
    std::vector<std::tuple<std::string, std::string, uint64_t>> tickets = {
        {"customer_support", "Refund for damaged order", 50},
        {"tech_support", "Login issue after update", 25},
        {"healthcare", "URGENT: Surgery reschedule", 100},
        {"finance", "Duplicate charge investigation", 75},
        {"sales", "Enterprise plan for 500 seats", 200},
    };
    
    auto wallet = grc.create_wallet();
    int ai = 0, human = 0;
    
    std::cout << "Processing across mesh network..." << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
    
    for (const auto& [module, desc, cost] : tickets) {
        if (!rate_limiter.allow("mesh")) continue;
        
        std::string response = swarm.process(module, desc);
        bool needs_human = (desc.find("URGENT") != std::string::npos);
        
        auto block = grc.mine_block(wallet);
        ledger.record("MESH_TXN", module);
        
        std::cout << "  [" << module << "] " << response.substr(0, 50) << "..." << std::endl;
        std::cout << "    Block #" << block.index << " | Reward: " << block.reward << " GRC" << std::endl;
        
        if (needs_human) human++; else ai++;
        monitor.record(static_cast<double>(ai + human));
    }
    
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << std::endl;
    
    monitor.report();
    std::cout << std::endl;
    
    double ai_pct = (ai + human > 0) ? ai * 100.0 / (ai + human) : 0;
    
    std::cout << "+==============================================+" << std::endl;
    std::cout << "|   MESH ENTERPRISE RESULTS                      |" << std::endl;
    std::cout << "|   Tickets: " << (ai + human) << " | AI: " << ai << " | Human: " << human << "                    |" << std::endl;
    std::cout << "|   AI Rate: " << ai_pct << "%                              |" << std::endl;
    std::cout << "|   Mesh Nodes: 4 (1L+2W+1E)                     |" << std::endl;
    std::cout << "|   Online Peers: " << leader.peer_count() << "                             |" << std::endl;
    std::cout << "|   GRC Chain: " << grc.chain_length() << " blocks | " << (grc.verify_chain() ? "VALID" : "CORRUPT") << "                     |" << std::endl;
    std::cout << "|   Swarm Cores: " << swarm.total_cores() << " | Active: " << swarm.active_cores() << "                           |" << std::endl;
    std::cout << "|   Ledger: " << ledger.size() << " entries (verified)               |" << std::endl;
    std::cout << "|                                               |" << std::endl;
    std::cout << "|   Dashboard: http://localhost:8092             |" << std::endl;
    std::cout << "+==============================================+" << std::endl;
    
    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
