#include <iostream>
#include "../src/phi_storage.h"

int main() {
    divine::PhiStorage db("/home/singularitynode/divine-bpo/divine_bpo.db");
    
    std::cout << "=== DATABASE CONTENTS ===" << std::endl;
    std::cout << "Sessions: " << db.query_int("SELECT COUNT(*) FROM sessions") << std::endl;
    std::cout << "Tickets: " << db.query_int("SELECT COUNT(*) FROM tickets") << std::endl;
    std::cout << "Ledger: " << db.query_int("SELECT COUNT(*) FROM ledger") << std::endl;
    std::cout << "Blocks: " << db.query_int("SELECT COUNT(*) FROM blocks") << std::endl;
    std::cout << "Wallets: " << db.query_int("SELECT COUNT(*) FROM wallets") << std::endl;
    std::cout << "Config: " << db.query_int("SELECT COUNT(*) FROM config") << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== SAMPLE DATA ===" << std::endl;
    std::cout << "Token: " << db.query_value("SELECT token FROM sessions LIMIT 1") << std::endl;
    std::cout << "User: " << db.query_value("SELECT user_id FROM sessions LIMIT 1") << std::endl;
    
    return 0;
}
