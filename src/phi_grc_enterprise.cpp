#include <iostream>
#include <thread>
#include <iomanip>
#include "phi_fhe_core.h"
#include "phi_http.h"
#include "phi_lyapunov.h"
#include "phi_rate_limiter.h"
#include "phi_circuit_breaker.h"
#include "phi_ledger.h"
#include "phi_swarm.h"
#include "phi_websocket.h"
#include "phi_grc.h"

int main() {
    std::cout << R"(
+==============================================+
|  DIVINE BPO -- GRC COIN ENTERPRISE v7.0       |
|  Golden Ratio Cryptocurrency + AI Swarm       |
+==============================================+
)" << std::endl;
    
    divine::PhiSwarm swarm;
    divine::LyapunovMonitor monitor;
    divine::RateLimiter rate_limiter;
    divine::CircuitBreaker circuit_breaker;
    divine::WebSocketServer ws(8093);
    PhiLedger ledger;
    PhiFHECore fhe_core;
    divine::GRCCoin grc;
    
    swarm.start(2);
    
    http::PhiServer http_server(8092);
    std::thread http_thread([&http_server]() { http_server.start(); });
    http_thread.detach();
    
    std::thread ws_thread([&ws]() { ws.start(); });
    ws_thread.detach();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ledger.record("GRC_INIT", "genesis_block");
    
    // Create wallets
    auto divine_wallet = grc.create_wallet();
    auto agent_wallet = grc.create_wallet();
    auto client_wallet = grc.create_wallet();
    
    std::cout << "GRC Coin Economy:" << std::endl;
    std::cout << "  Total Supply: " << grc.get_total_supply() << " GRC" << std::endl;
    std::cout << "  Block Time: 1618 ms (phi * 1000)" << std::endl;
    std::cout << "  Block Reward: 50 * phi^(-block/10000)" << std::endl;
    std::cout << std::endl;
    std::cout << "  Divine Wallet:  " << divine_wallet << std::endl;
    std::cout << "  Agent Wallet:   " << agent_wallet << std::endl;
    std::cout << "  Client Wallet:  " << client_wallet << std::endl;
    std::cout << std::endl;
    
    // Mine some blocks
    std::cout << "Mining blocks..." << std::endl;
    for (int i = 0; i < 5; i++) {
        auto block = grc.mine_block(divine_wallet);
        std::cout << "  Block #" << block.index << " | Reward: " << block.reward 
                  << " GRC | Hash: " << block.hash.substr(0, 16) << "..." << std::endl;
    }
    std::cout << std::endl;
    
    // Process BPO tickets, pay in GRC
    std::vector<std::tuple<std::string, std::string, uint64_t>> paid_tickets = {
        {"customer_support", "Refund for damaged order", 50},
        {"tech_support", "Login issue resolution", 25},
        {"healthcare", "URGENT: Surgery reschedule", 100},
        {"finance", "Duplicate charge investigation", 75},
        {"sales", "Enterprise demo completed", 200},
    };
    
    std::cout << "Processing paid tickets (GRC settlements)..." << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
    
    int ai_count = 0, human_count = 0;
    
    for (const auto& [module, desc, cost] : paid_tickets) {
        if (!rate_limiter.allow("grc_client")) continue;
        
        std::string ai_response = swarm.process(module, desc);
        bool needs_human = (desc.find("URGENT") != std::string::npos);
        
        // Transfer GRC
        std::string worker = needs_human ? agent_wallet : divine_wallet;
        grc.transfer(client_wallet, worker, cost);
        
        auto block = grc.mine_block(divine_wallet);
        ledger.record("GRC_PAYMENT", desc);
        
        std::cout << "  " << module << ": " << cost << " GRC";
        std::cout << " | Balance: " << grc.get_balance(client_wallet) << " GRC";
        std::cout << " | Block: #" << block.index << std::endl;
        
        if (needs_human) human_count++; else ai_count++;
        monitor.record(static_cast<double>(ai_count + human_count));
    }
    
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << std::endl;
    
    monitor.report();
    std::cout << std::endl;
    
    double ai_pct = (ai_count + human_count > 0) ? ai_count * 100.0 / (ai_count + human_count) : 0;
    
    std::cout << "+==============================================+" << std::endl;
    std::cout << "|   GRC COIN ENTERPRISE RESULTS                  |" << std::endl;
    std::cout << "|   Tickets: " << (ai_count + human_count) << " | AI: " << ai_count << " | Human: " << human_count << "                    |" << std::endl;
    std::cout << "|   AI Rate: " << ai_pct << "%                              |" << std::endl;
    std::cout << "|   GRC Chain: " << grc.chain_length() << " blocks                           |" << std::endl;
    std::cout << "|   GRC Verified: " << (grc.verify_chain() ? "VALID" : "CORRUPT") << "                         |" << std::endl;
    std::cout << "|   Divine Balance: " << grc.get_balance(divine_wallet) << " GRC                      |" << std::endl;
    std::cout << "|   Agent Balance: " << grc.get_balance(agent_wallet) << " GRC                       |" << std::endl;
    std::cout << "|   Client Balance: " << grc.get_balance(client_wallet) << " GRC                      |" << std::endl;
    std::cout << "|   Swarm Cores: " << swarm.total_cores() << " | Active: " << swarm.active_cores() << "                           |" << std::endl;
    std::cout << "|   Ledger: " << ledger.size() << " entries (verified)               |" << std::endl;
    std::cout << "|                                               |" << std::endl;
    std::cout << "|   Dashboard: http://localhost:8092             |" << std::endl;
    std::cout << "+==============================================+" << std::endl;
    
    while (true) std::this_thread::sleep_for(std::chrono::seconds(1));
    
    swarm.stop();
    return 0;
}
