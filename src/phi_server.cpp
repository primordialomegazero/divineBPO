#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "phi_core.h"

class DivineBPO {
private:
    PhiCore core;
    std::map<std::string, PhiToken> sessions;
    
public:
    void start() {
        std::cout << R"(
+==============================================+
|          DIVINE BPO -- CORE v1.0              |
|   Gateway + Database + Auth + Core            |
|   90% AI | 10% Human | Unlimited Scale        |
+==============================================+
)" << std::endl;
        
        std::cout << "Modules loaded: ";
        for (const auto& m : core.modules) std::cout << m << " ";
        std::cout << std::endl << std::endl;
        
        auto token = phi_jwt_create("client_001");
        sessions["client_001"] = token;
        std::cout << "[OK] Client authenticated: JWT issued" << std::endl;
        
        std::vector<std::pair<std::string, std::string>> test_tickets = {
            {"customer_support", "My order #12345 has not arrived"},
            {"tech_support", "Cannot login to my account"},
            {"healthcare", "Need to reschedule appointment"},
            {"finance", "Incorrect charge on my credit card"},
            {"hr", "PTO request for next week"},
            {"sales", "Interested in enterprise plan"},
            {"logistics", "Shipment delayed by 3 days"},
            {"ecommerce", "Return request for item #6789"},
            {"customer_support", "Refund not processed"},
            {"tech_support", "App crashes on startup"},
        };
        
        int ai_count = 0, human_count = 0;
        
        std::cout << std::endl << "Processing tickets..." << std::endl;
        std::cout << "------------------------------------------------" << std::endl;
        
        for (const auto& [module, desc] : test_tickets) {
            auto ticket_id = core.create_ticket("client_001", module, module, desc);
            auto result = core.process_ticket(ticket_id);
            auto ticket = core.get_ticket(ticket_id);
            
            std::cout << "  " << ticket_id << " | " << module << std::endl;
            std::cout << "    " << result << std::endl;
            
            if (ticket->needs_human) human_count++;
            else ai_count++;
        }
        
        std::cout << "------------------------------------------------" << std::endl;
        std::cout << std::endl;
        
        double ai_pct = ai_count * 100.0 / (ai_count + human_count);
        
        std::cout << "+==============================================+" << std::endl;
        std::cout << "|   RESULTS                                     |" << std::endl;
        std::cout << "|   Total tickets: " << (ai_count + human_count) << "                            |" << std::endl;
        std::cout << "|   AI handled:    " << ai_count << "                            |" << std::endl;
        std::cout << "|   Human escalated: " << human_count << "                           |" << std::endl;
        std::cout << "|   AI rate: " << ai_pct << "%                                |" << std::endl;
        std::cout << "|                                               |" << std::endl;
        std::cout << "|   Divine BPO: READY FOR CLIENTS               |" << std::endl;
        std::cout << "+==============================================+" << std::endl;
    }
};

int main() {
    DivineBPO bpo;
    bpo.start();
    return 0;
}
