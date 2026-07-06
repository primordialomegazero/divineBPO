#include <iostream>
#include "../src/phi_storage.h"
#include <ctime>

int main() {
    divine::PhiStorage db("test_insert.db");
    
    // Direct test - 8 simple inserts
    for (int i = 0; i < 8; i++) {
        auto us = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::string id = "TKT-" + std::to_string(us);
        
        char sql[512];
        snprintf(sql, sizeof(sql),
            "INSERT INTO tickets VALUES('%s','test','test','test','test','ai_resolved',0,%ld,'test')",
            id.c_str(), std::time(nullptr));
        db.execute(sql);
    }
    
    std::cout << "Inserted: " << db.query_int("SELECT COUNT(*) FROM tickets") << std::endl;
    return 0;
}
