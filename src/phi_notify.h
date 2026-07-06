// PHI-NOTIFY — Email notifications via sendmail
// Alerts for escalations, failures, daily reports

#pragma once
#include <string>
#include <cstdio>
#include <memory>
#include <array>

namespace divine {

class Notifier {
private:
    std::string from;
    std::string smtp_server;
    
public:
    Notifier(const std::string& sender = "divine-bpo@localhost",
             const std::string& smtp = "localhost")
        : from(sender), smtp_server(smtp) {}
    
    bool send(const std::string& to, const std::string& subject, 
              const std::string& body) {
        std::string cmd = "echo '" + body + "' | mail -s '" + subject + "' " + to + " 2>/dev/null";
        int result = system(cmd.c_str());
        return result == 0;
    }
    
    void alert_escalation(const std::string& ticket_id, const std::string& module,
                          const std::string& description) {
        std::string subject = "[DIVINE BPO] ESCALATION: " + ticket_id;
        std::string body = "Ticket: " + ticket_id + "\n"
                          "Module: " + module + "\n"
                          "Description: " + description + "\n"
                          "Status: Requires human attention\n\n"
                          "Dashboard: http://localhost:8092";
        send("admin@divinebpo.local", subject, body);
    }
    
    void alert_failure(const std::string& service, const std::string& error) {
        std::string subject = "[DIVINE BPO] FAILURE: " + service;
        std::string body = "Service: " + service + "\n"
                          "Error: " + error + "\n"
                          "Time: " + std::to_string(std::time(nullptr)) + "\n\n"
                          "Check logs: divine_bpo.log";
        send("admin@divinebpo.local", subject, body);
    }
    
    void daily_report(int tickets, int ai, int human, int failed, double ai_rate,
                      int grc_blocks, bool chain_valid) {
        std::string subject = "[DIVINE BPO] Daily Report";
        std::string body = "Divine BPO Daily Report\n"
                          "=======================\n"
                          "Tickets processed: " + std::to_string(tickets) + "\n"
                          "AI handled: " + std::to_string(ai) + "\n"
                          "Human escalated: " + std::to_string(human) + "\n"
                          "Failed: " + std::to_string(failed) + "\n"
                          "AI rate: " + std::to_string(ai_rate) + "%\n"
                          "GRC blocks: " + std::to_string(grc_blocks) + "\n"
                          "Chain valid: " + std::string(chain_valid ? "YES" : "NO") + "\n\n"
                          "Dashboard: http://localhost:8092";
        send("admin@divinebpo.local", subject, body);
    }
};

} // namespace divine
