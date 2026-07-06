// PHI-GRC — Golden Ratio Cryptocurrency
// Total supply: phi^2 * 1,000,000 = 2,618,034 GRC
// Block reward: 50 * phi^(-block/10000)
// Block time: phi * 1000 = 1618 ms

#pragma once
#include "phi_constants.h"
#include "phi_hash.h"
#include "phi_ledger.h"
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <cmath>

namespace divine {

struct Block {
    uint64_t index;
    uint64_t timestamp;
    std::string previous_hash;
    std::string hash;
    std::string validator;
    std::vector<std::string> transactions;
    uint64_t reward;
};

struct Wallet {
    std::string address;
    uint64_t balance;
    std::vector<std::string> transaction_history;
};

class GRCCoin {
private:
    std::vector<Block> chain;
    std::map<std::string, Wallet> wallets;
    uint64_t total_supply;
    uint64_t circulating;
    
    static constexpr uint64_t MAX_SUPPLY = 2618034;  // phi^2 * 1M
    static constexpr uint64_t BASE_REWARD = 50;
    static constexpr uint64_t BLOCK_TIME_MS = 1618;   // phi * 1000
    
    std::string compute_block_hash(const Block& block) const {
        std::string data = std::to_string(block.index) + 
                          std::to_string(block.timestamp) +
                          block.previous_hash + block.validator;
        for (const auto& tx : block.transactions) data += tx;
        return std::to_string(phi_hash(data));
    }
    
    uint64_t compute_reward(uint64_t block_index) const {
        return static_cast<uint64_t>(BASE_REWARD * std::pow(PHI_INV, block_index / 10000.0));
    }

public:
    GRCCoin() : total_supply(MAX_SUPPLY), circulating(0) {
        // Genesis block
        Block genesis{0, 0, "0", "", "divine_bpo_genesis", 
                      {"GENESIS: Divine BPO GRC Coin v1.0"}, 0};
        genesis.hash = compute_block_hash(genesis);
        chain.push_back(genesis);
        
        // Genesis wallet
        wallets["genesis"] = {"genesis", MAX_SUPPLY, {"GENESIS_ALLOCATION"}};
    }
    
    std::string create_wallet() {
        std::string address = "GRC-" + std::to_string(phi_hash(std::to_string(wallets.size())));
        wallets[address] = {address, 0, {}};
        return address;
    }
    
    bool transfer(const std::string& from, const std::string& to, uint64_t amount) {
        if (wallets.find(from) == wallets.end()) return false;
        if (wallets.find(to) == wallets.end()) return false;
        if (wallets[from].balance < amount) return false;
        
        wallets[from].balance -= amount;
        wallets[to].balance += amount;
        circulating += amount;
        
        std::string tx = from + "->" + to + ":" + std::to_string(amount);
        wallets[from].transaction_history.push_back(tx);
        wallets[to].transaction_history.push_back(tx);
        
        return true;
    }
    
    Block mine_block(const std::string& validator) {
        Block block;
        block.index = chain.size();
        block.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        block.previous_hash = chain.back().hash;
        block.validator = validator;
        block.reward = compute_reward(block.index);
        
        // Coinbase transaction
        block.transactions.push_back("COINBASE: " + validator + " +" + std::to_string(block.reward));
        
        block.hash = compute_block_hash(block);
        
        if (wallets.find(validator) != wallets.end()) {
            wallets[validator].balance += block.reward;
        }
        
        chain.push_back(block);
        return block;
    }
    
    uint64_t get_balance(const std::string& address) const {
        auto it = wallets.find(address);
        if (it == wallets.end()) return 0;
        return it->second.balance;
    }
    
    bool verify_chain() const {
        for (size_t i = 1; i < chain.size(); i++) {
            if (chain[i].previous_hash != chain[i-1].hash) return false;
            if (chain[i].hash != compute_block_hash(chain[i])) return false;
        }
        return true;
    }
    
    uint64_t chain_length() const { return chain.size(); }
    uint64_t get_circulating() const { return circulating; }
    uint64_t get_total_supply() const { return total_supply; }
    
    double inflation_rate() const {
        return (double)circulating / (double)total_supply * 100.0;
    }
    
    const Block& latest_block() const { return chain.back(); }
};

} // namespace divine
