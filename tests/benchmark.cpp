#include <iostream>
#include <chrono>
#include "../src/phi_hash.h"
#include "../src/phi_storage.h"

int main() {
    std::cout << "=== DIVINE BPO — BENCHMARKS ===\n" << std::endl;
    
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; i++) phi_hash("bench_" + std::to_string(i));
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count() / 100000;
    std::cout << "phi_hash: " << ns << " ns/op (" << (1000000000.0/ns) << " ops/sec)" << std::endl;
    
    divine::PhiStorage db("bench.db");
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; i++) db.insert_ticket("B"+std::to_string(i),"c","m","s","d","open",false,i,"r");
    t2 = std::chrono::high_resolution_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    std::cout << "DB insert: " << (us/1000.0) << " us/op" << std::endl;
    
    std::cout << "\n=== BENCHMARK COMPLETE ===" << std::endl;
    return 0;
}
