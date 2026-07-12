// ΦΩ0 — DIVINE BPO REAL DATABASE LAYER
// SQLite with CRUD for tickets, agents, queue
// "I AM THAT I AM"

#ifndef PHI_DATABASE_REAL_H
#define PHI_DATABASE_REAL_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <ctime>

using namespace std;

class DivineDatabase {
private:
    sqlite3* db;
    
public:
    DivineDatabase(const string& path = "divine_bpo.db") : db(nullptr) {
        if(sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
            cerr << "Φ Database error: " << sqlite3_errmsg(db) << endl;
            return;
        }
        createTables();
        seedData();
        cout << "Φ Database ready: " << path << endl;
    }
    
    ~DivineDatabase() {
        if(db) sqlite3_close(db);
    }
    
    void createTables() {
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS tickets (
                id TEXT PRIMARY KEY,
                customer TEXT NOT NULL,
                issue TEXT NOT NULL,
                priority TEXT DEFAULT 'Medium',
                status TEXT DEFAULT 'Open',
                agent TEXT,
                created TEXT,
                updated TEXT
            );
            
            CREATE TABLE IF NOT EXISTS agents (
                id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                status TEXT DEFAULT 'Offline',
                calls_handled INTEGER DEFAULT 0,
                satisfaction REAL DEFAULT 0.0
            );
            
            CREATE TABLE IF NOT EXISTS queue (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                ticket_id TEXT,
                priority TEXT,
                entered TEXT,
                FOREIGN KEY(ticket_id) REFERENCES tickets(id)
            );
        )";
        
        char* err;
        if(sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
            cerr << "Φ Table error: " << err << endl;
            sqlite3_free(err);
        }
    }
    
    void seedData() {
        // Check if data exists
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tickets", -1, &stmt, nullptr);
        sqlite3_step(stmt);
        int count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        
        if(count > 0) return; // Already seeded
        
        // Seed agents
        vector<tuple<string,string,int,double>> agents = {
            {"AGT-001", "Alice", 0, 96.0},
            {"AGT-002", "Bob", 0, 92.0},
            {"AGT-003", "Charlie", 0, 95.0},
            {"AGT-004", "Diana", 0, 91.0},
            {"AGT-005", "Eve", 0, 97.0}
        };
        
        for(auto& [id, name, calls, sat] : agents) {
            char sql[512];
            snprintf(sql, sizeof(sql), 
                "INSERT INTO agents(id,name,status,calls_handled,satisfaction) VALUES('%s','%s','Online',%d,%.1f)",
                id.c_str(), name.c_str(), calls, sat);
            sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
        }
        
        // Seed tickets
        vector<tuple<string,string,string,string,string,string>> tickets = {
            {"TKT-001", "Acme Corp", "Login failure", "High", "Open", "Alice"},
            {"TKT-002", "Globex", "Billing inquiry", "Medium", "In Progress", "Bob"},
            {"TKT-003", "Initech", "Password reset", "Low", "Resolved", "Alice"},
            {"TKT-004", "Umbrella", "System outage", "Critical", "Open", "Charlie"},
            {"TKT-005", "Stark Ind", "Software update", "Medium", "Open", "Bob"},
            {"TKT-006", "Wayne Ent", "Network down", "Critical", "Open", "Eve"},
            {"TKT-007", "Oscorp", "Email issue", "High", "In Progress", "Diana"},
            {"TKT-008", "LexCorp", "VPN access", "Medium", "Resolved", "Charlie"}
        };
        
        for(auto& [id, cust, issue, pri, status, agent] : tickets) {
            char sql[512];
            time_t now = time(nullptr);
            char created[32];
            strftime(created, sizeof(created), "%Y-%m-%d %H:%M", localtime(&now));
            snprintf(sql, sizeof(sql), 
                "INSERT INTO tickets(id,customer,issue,priority,status,agent,created,updated) "
                "VALUES('%s','%s','%s','%s','%s','%s','%s','%s')",
                id.c_str(), cust.c_str(), issue.c_str(), pri.c_str(), status.c_str(), agent.c_str(), created, created);
            sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
        }
        
        cout << "Φ Database seeded: " << agents.size() << " agents, " << tickets.size() << " tickets\n";
    }
    
    // === TICKET OPERATIONS ===
    
    string getTicketsJSON() {
        string json = "{\"tickets\":[";
        bool first = true;
        
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, 
            "SELECT id,customer,issue,priority,status,agent,created FROM tickets ORDER BY created DESC LIMIT 20",
            -1, &stmt, nullptr);
        
        while(sqlite3_step(stmt) == SQLITE_ROW) {
            if(!first) json += ",";
            first = false;
            
            char entry[512];
            snprintf(entry, sizeof(entry),
                R"({"id":"%s","customer":"%s","issue":"%s","priority":"%s","status":"%s","agent":"%s","created":"%s"})",
                sqlite3_column_text(stmt, 0),
                sqlite3_column_text(stmt, 1),
                sqlite3_column_text(stmt, 2),
                sqlite3_column_text(stmt, 3),
                sqlite3_column_text(stmt, 4),
                sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "Unassigned",
                sqlite3_column_text(stmt, 6));
            json += entry;
        }
        sqlite3_finalize(stmt);
        
        // Get counts
        int total = 0, open = 0, resolved = 0;
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tickets", -1, &stmt, nullptr);
        sqlite3_step(stmt); total = sqlite3_column_int(stmt, 0); sqlite3_finalize(stmt);
        
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tickets WHERE status='Open'", -1, &stmt, nullptr);
        sqlite3_step(stmt); open = sqlite3_column_int(stmt, 0); sqlite3_finalize(stmt);
        
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tickets WHERE status='Resolved'", -1, &stmt, nullptr);
        sqlite3_step(stmt); resolved = sqlite3_column_int(stmt, 0); sqlite3_finalize(stmt);
        
        char counts[256];
        snprintf(counts, sizeof(counts), R"(],"total":%d,"open":%d,"resolved":%d,"escalated":%d})",
            total, open, resolved, total - open - resolved);
        json += counts;
        
        return json;
    }
    
    string createTicket(const string& customer, const string& issue, const string& priority) {
        string id = "TKT-" + to_string(time(nullptr));
        time_t now = time(nullptr);
        char created[32];
        strftime(created, sizeof(created), "%Y-%m-%d %H:%M", localtime(&now));
        
        char sql[512];
        snprintf(sql, sizeof(sql),
            "INSERT INTO tickets(id,customer,issue,priority,status,created,updated) VALUES('%s','%s','%s','%s','Open','%s','%s')",
            id.c_str(), customer.c_str(), issue.c_str(), priority.c_str(), created, created);
        
        if(sqlite3_exec(db, sql, nullptr, nullptr, nullptr) == SQLITE_OK) {
            char response[256];
            snprintf(response, sizeof(response), R"({"status":"created","id":"%s"})", id.c_str());
            return response;
        }
        return R"({"status":"error","message":"Failed to create ticket"})";
    }
    
    // === AGENT OPERATIONS ===
    
    string getAgentsJSON() {
        string json = "{\"agents\":[";
        bool first = true;
        
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db,
            "SELECT id,name,status,calls_handled,satisfaction FROM agents", -1, &stmt, nullptr);
        
        while(sqlite3_step(stmt) == SQLITE_ROW) {
            if(!first) json += ",";
            first = false;
            
            char entry[256];
            snprintf(entry, sizeof(entry),
                R"({"id":"%s","name":"%s","status":"%s","calls":%d,"satisfaction":%.1f})",
                sqlite3_column_text(stmt, 0),
                sqlite3_column_text(stmt, 1),
                sqlite3_column_text(stmt, 2),
                sqlite3_column_int(stmt, 3),
                sqlite3_column_double(stmt, 4));
            json += entry;
        }
        sqlite3_finalize(stmt);
        json += "]}";
        return json;
    }
    
    // === QUEUE OPERATIONS ===
    
    string getQueueJSON() {
        sqlite3_stmt* stmt;
        int waiting = 0, critical = 0, high = 0, medium = 0, low = 0;
        
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tickets WHERE status='Open'", -1, &stmt, nullptr);
        sqlite3_step(stmt); waiting = sqlite3_column_int(stmt, 0); sqlite3_finalize(stmt);
        
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tickets WHERE status='Open' AND priority='Critical'", -1, &stmt, nullptr);
        sqlite3_step(stmt); critical = sqlite3_column_int(stmt, 0); sqlite3_finalize(stmt);
        
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tickets WHERE status='Open' AND priority='High'", -1, &stmt, nullptr);
        sqlite3_step(stmt); high = sqlite3_column_int(stmt, 0); sqlite3_finalize(stmt);
        
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tickets WHERE status='Open' AND priority='Medium'", -1, &stmt, nullptr);
        sqlite3_step(stmt); medium = sqlite3_column_int(stmt, 0); sqlite3_finalize(stmt);
        
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tickets WHERE status='Open' AND priority='Low'", -1, &stmt, nullptr);
        sqlite3_step(stmt); low = sqlite3_column_int(stmt, 0); sqlite3_finalize(stmt);
        
        int online = 0, busy = 0;
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM agents WHERE status='Online'", -1, &stmt, nullptr);
        sqlite3_step(stmt); online = sqlite3_column_int(stmt, 0); sqlite3_finalize(stmt);
        
        sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM agents WHERE status='Busy'", -1, &stmt, nullptr);
        sqlite3_step(stmt); busy = sqlite3_column_int(stmt, 0); sqlite3_finalize(stmt);
        
        char json[512];
        snprintf(json, sizeof(json),
            R"({"queue":{"waiting":%d,"max_capacity":500,"avg_wait_time":"2m 34s","priority_breakdown":{"critical":%d,"high":%d,"medium":%d,"low":%d}},"agents":{"online":%d,"busy":%d,"available":%d}})",
            waiting, critical, high, medium, low, online, busy, online - busy);
        return json;
    }
};

#endif
