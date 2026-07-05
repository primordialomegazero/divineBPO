// PHI-AI v2 — Direct Ollama HTTP API
// No shell escaping issues. Proper JSON handling.

#pragma once
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <ctime>

namespace divine {

class PhiAI {
private:
    bool ollama_available;
    
    std::string http_post(const std::string& host, int port,
                          const std::string& path, const std::string& body) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return "";
        
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
        
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            return "";
        }
        
        std::stringstream request;
        request << "POST " << path << " HTTP/1.1\r\n"
                << "Host: " << host << ":" << port << "\r\n"
                << "Content-Type: application/json\r\n"
                << "Content-Length: " << body.size() << "\r\n"
                << "Connection: close\r\n\r\n"
                << body;
        
        std::string req = request.str();
        send(sock, req.c_str(), req.size(), 0);
        
        std::string response;
        char buffer[4096];
        ssize_t bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytes] = '\0';
            response += buffer;
        }
        close(sock);
        
        // Extract body from HTTP response
        size_t body_start = response.find("\r\n\r\n");
        if (body_start == std::string::npos) return "";
        return response.substr(body_start + 4);
    }
    
    std::string json_escape(const std::string& s) {
        std::string out;
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:   out += c;
            }
        }
        return out;
    }
    
    std::string extract_response(const std::string& json) {
        // Simple JSON parsing for Ollama response
        size_t pos = json.find("\"response\":\"");
        if (pos == std::string::npos) return "";
        pos += 12;
        
        std::string result;
        bool escaping = false;
        for (size_t i = pos; i < json.size(); i++) {
            char c = json[i];
            if (escaping) {
                result += c;
                escaping = false;
                continue;
            }
            if (c == '\\') {
                escaping = true;
                continue;
            }
            if (c == '"') break;
            result += c;
        }
        return result;
    }
    
public:
    PhiAI() {
        // Test Ollama connection
        std::string response = http_post("127.0.0.1", 11434, "/api/generate",
            R"({"model":"llama3.2","prompt":"test","stream":false})");
        ollama_available = !response.empty() && response.find("\"response\"") != std::string::npos;
    }
    
    bool is_available() const { return ollama_available; }
    
    std::string process_ticket(const std::string& module,
                                const std::string& description) {
        if (!ollama_available) {
            return process_rule_based(module, description);
        }
        
        std::string prompt = "You are a professional " + module + " support agent. "
            "Respond to this customer inquiry in 1-2 sentences. "
            "Be concise, helpful, and professional.\n\n"
            "Customer: " + description + "\n\nAgent:";
        
        std::string body = R"({"model":"llama3.2","prompt":")" 
                         + json_escape(prompt) 
                         + R"(","stream":false})";
        
        std::string response = http_post("127.0.0.1", 11434, "/api/generate", body);
        
        if (response.empty()) {
            ollama_available = false;
            return process_rule_based(module, description);
        }
        
        std::string ai_text = extract_response(response);
        if (ai_text.empty()) {
            return process_rule_based(module, description);
        }
        
        return ai_text;
    }
    
    std::string classify_module(const std::string& description) {
        std::string desc_lower = description;
        for (auto& c : desc_lower) c = tolower(c);
        
        if (desc_lower.find("order") != std::string::npos ||
            desc_lower.find("refund") != std::string::npos ||
            desc_lower.find("deliver") != std::string::npos) return "customer_support";
        if (desc_lower.find("login") != std::string::npos ||
            desc_lower.find("password") != std::string::npos ||
            desc_lower.find("app") != std::string::npos) return "tech_support";
        if (desc_lower.find("doctor") != std::string::npos ||
            desc_lower.find("surgery") != std::string::npos ||
            desc_lower.find("appointment") != std::string::npos) return "healthcare";
        if (desc_lower.find("charge") != std::string::npos ||
            desc_lower.find("payment") != std::string::npos ||
            desc_lower.find("invoice") != std::string::npos) return "finance";
        if (desc_lower.find("hire") != std::string::npos ||
            desc_lower.find("pto") != std::string::npos ||
            desc_lower.find("salary") != std::string::npos) return "hr";
        if (desc_lower.find("enterprise") != std::string::npos ||
            desc_lower.find("plan") != std::string::npos) return "sales";
        
        return "customer_support";
    }
    
    bool needs_human(const std::string& description) {
        std::string desc_lower = description;
        for (auto& c : desc_lower) c = tolower(c);
        
        if (desc_lower.find("urgent") != std::string::npos) return true;
        if (desc_lower.find("emergency") != std::string::npos) return true;
        if (desc_lower.find("lawsuit") != std::string::npos) return true;
        if (desc_lower.find("manager") != std::string::npos) return true;
        if (desc_lower.find("supervisor") != std::string::npos) return true;
        
        return (rand() % 10 == 0);
    }
    
private:
    std::string process_rule_based(const std::string& module,
                                    const std::string& description) {
        std::string desc_lower = description;
        for (auto& c : desc_lower) c = tolower(c);
        
        if (desc_lower.find("refund") != std::string::npos ||
            desc_lower.find("charge") != std::string::npos) {
            return "Payment concern logged. Specialist reviews within 24h. Ref: " + gen_ref();
        }
        if (desc_lower.find("login") != std::string::npos ||
            desc_lower.find("password") != std::string::npos) {
            return "Account issue noted. Password reset sent to registered email.";
        }
        if (desc_lower.find("order") != std::string::npos ||
            desc_lower.find("deliver") != std::string::npos) {
            return "Order status: active processing. Tracking updates via email.";
        }
        if (desc_lower.find("appointment") != std::string::npos ||
            desc_lower.find("schedule") != std::string::npos) {
            return "Appointment request received. Available slots sent. Confirm within 48h.";
        }
        return "Inquiry received. Team responds within 4h. Ref: " + gen_ref();
    }
    
    std::string gen_ref() {
        char ref[12];
        snprintf(ref, sizeof(ref), "PHI-%04X", rand() & 0xFFFF);
        return std::string(ref);
    }
};

} // namespace divine
