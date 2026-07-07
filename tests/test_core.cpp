#include <iostream>
#include <cassert>
#include "../src/phi_hash.h"
#include "../src/phi_storage.h"
#include "../src/phi_validate.h"
#include "../src/phi_config.h"
#include "../src/phi_jwt.h"

int passed = 0, failed = 0;
void test(const std::string& name, bool condition) {
    if (condition) { passed++; std::cout << "  [PASS] " << name << std::endl; }
    else { failed++; std::cout << "  [FAIL] " << name << std::endl; }
}

int main() {
    std::cout << "=== DIVINE BPO — UNIT TESTS ===\n" << std::endl;
    
    std::cout << "phi_hash:" << std::endl;
    test("hash non-empty", phi_hash("test") != 0);
    test("hash deterministic", phi_hash("test") == phi_hash("test"));
    test("hash different inputs", phi_hash("a") != phi_hash("b"));
    
    std::cout << "\nphi_storage:" << std::endl;
    divine::PhiStorage db("test_unit.db");
    db.insert_ticket("T1", "c1", "support", "test", "desc", "open", false, 123456, "res");
    test("ticket inserted", db.query_int("SELECT COUNT(*) FROM tickets") == 1);
    
    std::cout << "\nphi_validate:" << std::endl;
    divine::Validator v;
    test("valid module", v.is_valid_module("customer_support"));
    test("invalid module", !v.is_valid_module("hack"));
    
    std::cout << "\nphi_jwt:" << std::endl;
    auto token = phi_jwt_create("admin");
    test("token created", !token.signature.empty());
    test("token verifies", phi_jwt_verify(token));
    
    std::cout << "\nphi_config:" << std::endl;
    divine::PhiConfig cfg(db);
    cfg.load_defaults();
    test("config loaded", cfg.get("app_name") == "Divine BPO");
    
    std::cout << "\n=== RESULTS: " << passed << "/" << (passed + failed) << " PASSED ===" << std::endl;
    return failed > 0 ? 1 : 0;
}
