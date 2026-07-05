// PHI-LEDGER — Immutable audit trail
// Every transaction recorded, phi-verifiable

#pragma once
#include "phi_hash.h"
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>

struct LedgerEntry {
    uint64_t index;
    uint64_t timestamp;
    std::string operation;
    std::string entity_id;
    std::string previous_hash;
    std::string current_hash;
};

class PhiLedger {
private:
    std::vector<LedgerEntry> chain;
    
    std::string compute_hash(const LedgerEntry& entry) const {
        std::stringstream ss;
        ss << entry.index << entry.timestamp << entry.operation 
           << entry.entity_id << entry.previous_hash;
        uint64_t h = phi_hash(ss.str());
        return std::to_string(h);
    }
    
public:
    PhiLedger() {
        // Genesis block
        LedgerEntry genesis{0, 0, "GENESIS", "divine_bpo_v1", "0", ""};
        genesis.current_hash = compute_hash(genesis);
        chain.push_back(genesis);
    }
    
    void record(const std::string& operation, const std::string& entity_id) {
        LedgerEntry entry;
        entry.index = chain.size();
        entry.timestamp = static_cast<uint64_t>(std::time(nullptr));
        entry.operation = operation;
        entry.entity_id = entity_id;
        entry.previous_hash = chain.back().current_hash;
        entry.current_hash = compute_hash(entry);
        chain.push_back(entry);
    }
    
    bool verify() const {
        for (size_t i = 1; i < chain.size(); i++) {
            if (chain[i].previous_hash != chain[i-1].current_hash) return false;
            if (chain[i].current_hash != compute_hash(chain[i])) return false;
        }
        return true;
    }
    
    size_t size() const { return chain.size(); }
    const LedgerEntry& last() const { return chain.back(); }
};
