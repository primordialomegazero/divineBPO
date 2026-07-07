// PHI-DBPOOL — Connection Pool + Redis Cache
// Thread-safe SQLite pool + Redis caching layer

#pragma once
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <sqlite3.h>

namespace divine {

class DBConnection {
public:
    sqlite3* db;
    bool in_use;
    
    DBConnection(const std::string& path) : in_use(false) {
        sqlite3_open(path.c_str(), &db);
        sqlite3_exec(db, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA busy_timeout=5000", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA cache_size=-8000", nullptr, nullptr, nullptr);
    }
    
    ~DBConnection() { if (db) sqlite3_close(db); }
};

class DBPool {
private:
    std::vector<std::unique_ptr<DBConnection>> pool;
    std::queue<size_t> available;
    std::mutex mtx;
    std::string db_path;
    
public:
    DBPool(const std::string& path, int size = 8) : db_path(path) {
        for (int i = 0; i < size; i++) {
            pool.push_back(std::make_unique<DBConnection>(path));
            available.push(i);
        }
    }
    
    DBConnection* acquire() {
        std::unique_lock<std::mutex> lock(mtx);
        if (available.empty()) {
            // Expand pool
            size_t idx = pool.size();
            pool.push_back(std::make_unique<DBConnection>(db_path));
            available.push(idx);
        }
        size_t idx = available.front();
        available.pop();
        pool[idx]->in_use = true;
        return pool[idx].get();
    }
    
    void release(DBConnection* conn) {
        std::unique_lock<std::mutex> lock(mtx);
        conn->in_use = false;
        for (size_t i = 0; i < pool.size(); i++) {
            if (pool[i].get() == conn) {
                available.push(i);
                return;
            }
        }
    }
    
    int query_int(const std::string& sql) {
        auto* conn = acquire();
        sqlite3_stmt* stmt;
        int result = 0;
        if (sqlite3_prepare_v2(conn->db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                result = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
        release(conn);
        return result;
    }
    
    size_t size() const { return pool.size(); }
    size_t available_count() const { return available.size(); }
};

// Redis cache wrapper
class RedisCache {
private:
    bool connected;
    
    static std::string exec(const std::string& cmd) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        char buf[256];
        std::string result;
        while (fgets(buf, sizeof(buf), pipe)) result += buf;
        pclose(pipe);
        return result;
    }
    
public:
    RedisCache() {
        connected = (system("redis-cli ping 2>/dev/null | grep -q PONG") == 0);
    }
    
    bool is_available() const { return connected; }
    
    void set(const std::string& key, const std::string& value, int ttl = 60) {
        if (!connected) return;
        std::string cmd = "redis-cli SETEX " + key + " " + std::to_string(ttl) + " '" + value + "' 2>/dev/null";
        system(cmd.c_str());
    }
    
    std::string get(const std::string& key) {
        if (!connected) return "";
        return exec("redis-cli GET " + key + " 2>/dev/null");
    }
    
    void del(const std::string& key) {
        if (!connected) return;
        std::string cmd = "redis-cli DEL " + key + " 2>/dev/null";
        system(cmd.c_str());
    }
};

} // namespace divine
