#pragma once
#include "phi_constants.h"
#include <vector>
#include <string>
#include <mutex>
#include <optional>

template<typename T>
class PhiDatabase {
private:
    struct Entry {
        std::string key;
        T value;
        bool occupied = false;
        bool deleted = false;
    };
    
    std::vector<Entry> table;
    size_t capacity;
    size_t size_;
    std::mutex mtx;
    
    size_t probe_sequence(const std::string& key, size_t i) const {
        uint64_t h = 0;
        for (char c : key) h = h * 31 + static_cast<uint64_t>(c);
        return (h + divine::phi_probe(i, capacity)) % capacity;
    }
    
public:
    PhiDatabase(size_t cap = 65537) : capacity(cap), size_(0) {
        table.resize(capacity);
    }
    
    void insert(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mtx);
        for (size_t i = 0; i < capacity; i++) {
            size_t idx = probe_sequence(key, i);
            if (!table[idx].occupied || table[idx].deleted || table[idx].key == key) {
                table[idx].key = key;
                table[idx].value = value;
                table[idx].occupied = true;
                table[idx].deleted = false;
                size_++;
                return;
            }
        }
    }
    
    std::optional<T> get(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        for (size_t i = 0; i < capacity; i++) {
            size_t idx = probe_sequence(key, i);
            if (!table[idx].occupied) return std::nullopt;
            if (!table[idx].deleted && table[idx].key == key) return table[idx].value;
        }
        return std::nullopt;
    }
    
    bool remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mtx);
        for (size_t i = 0; i < capacity; i++) {
            size_t idx = probe_sequence(key, i);
            if (!table[idx].occupied) return false;
            if (!table[idx].deleted && table[idx].key == key) {
                table[idx].deleted = true;
                size_--;
                return true;
            }
        }
        return false;
    }
    
    size_t size() const { return size_; }
};
