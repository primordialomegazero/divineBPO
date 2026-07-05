#pragma once
#include "phi_database.h"
#include "phi_jwt.h"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <ctime>

struct Ticket {
    std::string id;
    std::string client_id;
    std::string module;
    std::string subject;
    std::string description;
    std::string status;
    bool needs_human;
    uint64_t created_at;
    std::string resolution;
};

class PhiCore {
private:
    PhiDatabase<Ticket> tickets;
    uint64_t ticket_counter = 0;
    
public:
    std::vector<std::string> modules = {
        "customer_support", "tech_support", "sales", "healthcare",
        "finance", "hr", "logistics", "telco", "ecommerce",
        "travel", "hospitality", "education", "real_estate", "legal"
    };
    
    std::string create_ticket(const std::string& client_id,
                               const std::string& module,
                               const std::string& subject,
                               const std::string& description) {
        Ticket ticket;
        ticket.id = "TKT-" + std::to_string(++ticket_counter);
        ticket.client_id = client_id;
        ticket.module = module;
        ticket.subject = subject;
        ticket.description = description;
        ticket.status = "open";
        ticket.created_at = static_cast<uint64_t>(std::time(nullptr));
        
        uint64_t h = phi_hash(description);
        ticket.needs_human = (h % 10 == 0);
        
        tickets.insert(ticket.id, ticket);
        return ticket.id;
    }
    
    std::optional<Ticket> get_ticket(const std::string& ticket_id) {
        return tickets.get(ticket_id);
    }
    
    std::string process_ticket(const std::string& ticket_id) {
        auto opt = tickets.get(ticket_id);
        if (!opt) return "Ticket not found";
        
        auto ticket = *opt;
        
        if (ticket.needs_human && ticket.status != "escalated") {
            ticket.status = "escalated";
            ticket.resolution = "Transferred to human agent. ETA: 15 minutes.";
            tickets.insert(ticket_id, ticket);
            return "ESCALATED TO HUMAN: " + ticket.resolution;
        }
        
        ticket.status = "ai_handled";
        ticket.resolution = "Resolved by AI. Module: " + ticket.module;
        tickets.insert(ticket_id, ticket);
        
        return "AI RESOLVED: " + ticket.resolution;
    }
    
    size_t ticket_count() const { return tickets.size(); }
};
