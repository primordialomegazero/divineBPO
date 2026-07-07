// PHI-AGI — AGI-like AI with Voice/Twilio Integration
// Multi-modal, context-aware, Twilio-ready

#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <thread>
#include <chrono>

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace divine {

class PhiAGI {
private:
    bool ollama_available;
    std::string ollama_host;
    int ollama_port;
    std::map<std::string, std::vector<std::string>> conversation_history;

    static std::string http_post(const std::string& host, int port, const std::string& path, const std::string& body) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return "";

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) { close(sock); return ""; }

        std::string req = "POST " + path + " HTTP/1.1\r\nHost: " + host + "\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
        send(sock, req.c_str(), req.size(), 0);

        char buf[8192] = {0};
        std::string response;
        while (recv(sock, buf, sizeof(buf) - 1, 0) > 0) {
            response += buf;
        }
        close(sock);
        return response;
    }

public:
    PhiAGI() : ollama_available(false), ollama_host("127.0.0.1"), ollama_port(11434) {}

    bool init() {
        std::string response = http_post(ollama_host, ollama_port, "/api/generate", 
            "{\"model\":\"llama3\",\"prompt\":\"Hello\",\"stream\":false}");
        ollama_available = !response.empty() && response.find("\"response\"") != std::string::npos;
        return ollama_available;
    }

    bool is_available() const { return ollama_available; }

    // AGI-like: context-aware response with memory
    std::string generate(const std::string& prompt, const std::string& conversation_id = "default") {
        if (!ollama_available) {
            return fallback_response(prompt);
        }

        // Build context from history
        std::string context = "You are an enterprise BPO AI assistant. Be professional, concise, and helpful.\n\n";
        auto& history = conversation_history[conversation_id];
        for (size_t i = 0; i < history.size() && i < 6; i++) {
            context += history[i] + "\n";
        }
        context += "User: " + prompt + "\nAssistant:";

        std::string body = "{\"model\":\"llama3\",\"prompt\":\"" + escape_json(context) + "\",\"stream\":false}";
        std::string response = http_post(ollama_host, ollama_port, "/api/generate", body);

        if (response.empty()) {
            ollama_available = false;
            return fallback_response(prompt);
        }

        // Extract response
        size_t pos = response.find("\"response\":\"");
        if (pos == std::string::npos) return fallback_response(prompt);
        
        pos += 12;
        size_t end = response.find("\"", pos);
        std::string result = response.substr(pos, end - pos);

        // Update history
        history.push_back("User: " + prompt);
        history.push_back("Assistant: " + result);

        return unescape_json(result);
    }

    // Twilio Voice-ready: Generate TwiML response
    std::string generate_twiml(const std::string& message) {
        return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
               "<Response>"
               "<Say voice=\"alice\">" + escape_xml(message) + "</Say>"
               "<Gather input=\"speech\" timeout=\"5\" action=\"/api/voice/process\" method=\"POST\">"
               "<Say voice=\"alice\">How can I help you?</Say>"
               "</Gather>"
               "</Response>";
    }

    // Twilio webhook handler
    std::string handle_voice_call(const std::string& speech_result, const std::string& call_sid) {
        std::string ai_response = generate(speech_result, "voice_" + call_sid);
        return generate_twiml(ai_response);
    }

    std::string classify_module(const std::string& description) {
        std::string desc_lower = description;
        for (char& c : desc_lower) c = tolower(c);

        if (desc_lower.find("billing") != std::string::npos || desc_lower.find("invoice") != std::string::npos) return "finance";
        if (desc_lower.find("password") != std::string::npos || desc_lower.find("login") != std::string::npos || desc_lower.find("vpn") != std::string::npos) return "tech_support";
        if (desc_lower.find("order") != std::string::npos || desc_lower.find("refund") != std::string::npos) return "customer_support";
        if (desc_lower.find("appointment") != std::string::npos || desc_lower.find("doctor") != std::string::npos) return "healthcare";
        if (desc_lower.find("pto") != std::string::npos || desc_lower.find("payroll") != std::string::npos) return "hr";
        if (desc_lower.find("product") != std::string::npos || desc_lower.find("pricing") != std::string::npos) return "sales";
        
        return "general";
    }

private:
    std::string fallback_response(const std::string& prompt) {
        std::string lower = prompt;
        for (char& c : lower) c = tolower(c);

        if (lower.find("hello") != std::string::npos || lower.find("hi") != std::string::npos)
            return "Hello! I'm Divine BPO AI. How can I assist you today?";
        if (lower.find("ticket") != std::string::npos || lower.find("status") != std::string::npos)
            return "Your ticket is being processed. Current AI handling rate is " + std::to_string((int)87.5) + "%.";
        if (lower.find("refund") != std::string::npos || lower.find("billing") != std::string::npos)
            return "I understand your billing concern. Let me create a ticket for our finance team.";
        if (lower.find("help") != std::string::npos)
            return "I'm here to help! I can assist with billing, technical support, scheduling, and general inquiries.";
        
        return "Thank you for contacting us. A team member will respond within 4 hours. Reference: TKT-" + std::to_string(rand() % 9000 + 1000);
    }

    static std::string escape_json(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else out += c;
        }
        return out;
    }

    static std::string unescape_json(const std::string& s) {
        std::string out;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == '\\' && i + 1 < s.size()) {
                if (s[i+1] == 'n') { out += '\n'; i++; }
                else if (s[i+1] == '"') { out += '"'; i++; }
                else out += s[i];
            } else out += s[i];
        }
        return out;
    }

    static std::string escape_xml(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '<') out += "&lt;";
            else if (c == '>') out += "&gt;";
            else if (c == '&') out += "&amp;";
            else if (c == '"') out += "&quot;";
            else out += c;
        }
        return out;
    }
};

} // namespace divine
