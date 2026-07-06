#pragma once
#include <sqlite3.h>
#include <string>
#include <mutex>
#include <iostream>
#include <ctime>

namespace divine {

class PhiStorage {
private:
    sqlite3* db;
    std::mutex mtx;

    std::string escape(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '\'') out += "''";
            else out += c;
        }
        return out;
    }

public:
    PhiStorage(const std::string& path = "divine_bpo.db") : db(nullptr) {
        if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) return;
        const char* schema = R"(
            CREATE TABLE IF NOT EXISTS tickets(id TEXT PRIMARY KEY, client_id TEXT, module TEXT, subject TEXT, description TEXT, status TEXT, needs_human INTEGER, created_at INTEGER, resolution TEXT);
            CREATE TABLE IF NOT EXISTS ledger(id INTEGER PRIMARY KEY AUTOINCREMENT, timestamp INTEGER, operation TEXT, entity_id TEXT, hash TEXT);
            CREATE TABLE IF NOT EXISTS wallets(address TEXT PRIMARY KEY, balance INTEGER, created_at INTEGER);
            CREATE TABLE IF NOT EXISTS blocks(index_num INTEGER PRIMARY KEY, timestamp INTEGER, previous_hash TEXT, hash TEXT, validator TEXT, reward INTEGER);
            CREATE TABLE IF NOT EXISTS sessions(token TEXT PRIMARY KEY, user_id TEXT, created_at INTEGER, expires_at INTEGER);
            CREATE TABLE IF NOT EXISTS config(key TEXT PRIMARY KEY, value TEXT);
        )";
        char* err = nullptr;
        sqlite3_exec(db, schema, nullptr, nullptr, &err);
        if (err) { sqlite3_free(err); }
        std::cout << "[STORAGE] SQLite ready" << std::endl;
    }

    ~PhiStorage() { if (db) sqlite3_close(db); }

    void execute(const std::string& sql) {
        std::lock_guard<std::mutex> lock(mtx);
        char* err = nullptr;
        sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
        if (err) { std::cerr << "[SQL] " << err << std::endl; sqlite3_free(err); }
    }

    std::string query_value(const std::string& sql) {
        std::lock_guard<std::mutex> lock(mtx);
        sqlite3_stmt* stmt = nullptr;
        std::string result;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* text = (const char*)sqlite3_column_text(stmt, 0);
                if (text) result = text;
            }
        }
        sqlite3_finalize(stmt);
        return result;
    }

    int query_int(const std::string& sql) {
        std::string val = query_value(sql);
        return val.empty() ? 0 : std::stoi(val);
    }

    void insert_ticket(const std::string& id, const std::string& client,
                       const std::string& module, const std::string& subject,
                       const std::string& desc, const std::string& status,
                       bool human, time_t now, const std::string& resolution) {
        std::string sql = "INSERT INTO tickets VALUES('" + escape(id) + "','" + 
                         escape(client) + "','" + escape(module) + "','" + 
                         escape(subject) + "','" + escape(desc) + "','" + 
                         escape(status) + "'," + (human ? "1" : "0") + "," + 
                         std::to_string(now) + ",'" + escape(resolution) + "')";
        execute(sql);
    }
};

} // namespace divine
