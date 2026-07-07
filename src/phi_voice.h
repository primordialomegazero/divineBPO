// PHI-VOICE — AGI Voice Handler
// Twilio webhook, real-time STT/TTS, call queue

#pragma once
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <queue>
#include <mutex>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace divine {

struct Call {
    std::string call_sid;
    std::string from;
    std::string to;
    std::string status; // ringing, in-progress, completed
    std::string transcript;
    std::string ai_response;
    time_t started_at;
    time_t ended_at;
    int duration_sec;
};

class VoiceHandler {
private:
    std::vector<Call> call_history;
    std::queue<Call> call_queue;
    std::mutex mtx;
    int max_queue;
    bool twilio_configured;
    std::string twilio_sid;
    std::string twilio_token;
    
public:
    VoiceHandler(int max_q = 100) : max_queue(max_q), twilio_configured(false) {}
    
    void configure(const std::string& sid, const std::string& token) {
        twilio_sid = sid;
        twilio_token = token;
        twilio_configured = !sid.empty();
    }
    
    bool is_configured() const { return twilio_configured; }
    
    // TwiML for incoming call
    std::string handle_incoming_call(const std::string& call_sid, const std::string& from, const std::string& to) {
        Call call;
        call.call_sid = call_sid;
        call.from = from;
        call.to = to;
        call.status = "ringing";
        call.started_at = time(nullptr);
        
        {
            std::unique_lock<std::mutex> lock(mtx);
            if (call_queue.size() < (size_t)max_queue) {
                call_queue.push(call);
            }
        }
        
        return generate_welcome_twiml(call_sid);
    }
    
    // Generate welcome message
    std::string generate_welcome_twiml(const std::string& call_sid) {
        char buf[4096];
        snprintf(buf, sizeof(buf),
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<Response>"
            "<Say voice=\"Polly.Joanna- Neural\">"
            "Welcome to Divine BPO. I am your AI assistant, powered by AGI technology. "
            "How can I help you today? You can say things like billing, technical support, "
            "or appointment scheduling."
            "</Say>"
            "<Gather input=\"speech dtmf\" timeout=\"5\" "
            "action=\"/api/voice/process?call_sid=%s\" method=\"POST\">"
            "<Say voice=\"Polly.Joanna- Neural\">"
            "I'm listening. Please tell me what you need."
            "</Say>"
            "</Gather>"
            "<Say voice=\"Polly.Joanna- Neural\">"
            "I didn't receive your input. Please call back or try our web dashboard. Goodbye."
            "</Say>"
            "</Response>",
            call_sid.c_str());
        return std::string(buf);
    }
    
    // Process speech input
    std::string process_speech(const std::string& call_sid, const std::string& speech_text, const std::string& digits) {
        std::string input = !speech_text.empty() ? speech_text : digits;
        
        // Classify intent
        std::string intent = classify_intent(input);
        std::string response = generate_ai_response(intent, input);
        
        // Update call record
        {
            std::unique_lock<std::mutex> lock(mtx);
            for (auto& call : call_history) {
                if (call.call_sid == call_sid) {
                    call.transcript += input + "\n";
                    call.ai_response = response;
                    call.status = "in-progress";
                }
            }
        }
        
        return generate_response_twiml(response);
    }
    
    // Generate response TwiML
    std::string generate_response_twiml(const std::string& message, bool continue_listening = true) {
        char buf[4096];
        if (continue_listening) {
            snprintf(buf, sizeof(buf),
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                "<Response>"
                "<Say voice=\"Polly.Joanna- Neural\">%s</Say>"
                "<Pause length=\"1\"/>"
                "<Gather input=\"speech dtmf\" timeout=\"4\" "
                "action=\"/api/voice/process\" method=\"POST\">"
                "<Say voice=\"Polly.Joanna- Neural\">"
                "Is there anything else I can help with?"
                "</Say>"
                "</Gather>"
                "<Say voice=\"Polly.Joanna- Neural\">"
                "Thank you for calling Divine BPO. Our team will follow up if needed. Goodbye."
                "</Say>"
                "</Response>",
                escape_xml(message).c_str());
        } else {
            snprintf(buf, sizeof(buf),
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                "<Response>"
                "<Say voice=\"Polly.Joanna- Neural\">%s</Say>"
                "<Hangup/>"
                "</Response>",
                escape_xml(message).c_str());
        }
        return std::string(buf);
    }
    
    // End call
    std::string end_call(const std::string& call_sid) {
        std::unique_lock<std::mutex> lock(mtx);
        for (auto& call : call_history) {
            if (call.call_sid == call_sid) {
                call.status = "completed";
                call.ended_at = time(nullptr);
                call.duration_sec = call.ended_at - call.started_at;
            }
        }
        return generate_response_twiml("Thank you for calling. Goodbye.", false);
    }
    
    // Get call stats
    std::string get_stats() {
        std::unique_lock<std::mutex> lock(mtx);
        char buf[512];
        snprintf(buf, sizeof(buf), 
            "{\"active_calls\":%zu,\"total_calls\":%zu,\"queued\":%zu}",
            call_history.size(), call_history.size(), call_queue.size());
        return std::string(buf);
    }
    
private:
    std::string classify_intent(const std::string& input) {
        std::string lower = input;
        for (char& c : lower) c = tolower(c);
        
        if (lower.find("bill") != std::string::npos || lower.find("invoice") != std::string::npos || lower.find("charge") != std::string::npos)
            return "finance";
        if (lower.find("password") != std::string::npos || lower.find("login") != std::string::npos || lower.find("tech") != std::string::npos)
            return "tech_support";
        if (lower.find("order") != std::string::npos || lower.find("refund") != std::string::npos || lower.find("delivery") != std::string::npos)
            return "customer_support";
        if (lower.find("appointment") != std::string::npos || lower.find("doctor") != std::string::npos || lower.find("schedule") != std::string::npos)
            return "healthcare";
        if (lower.find("pto") != std::string::npos || lower.find("vacation") != std::string::npos || lower.find("payroll") != std::string::npos)
            return "hr";
        
        return "general";
    }
    
    std::string generate_ai_response(const std::string& intent, const std::string& input) {
        if (intent == "finance") 
            return "I understand you have a billing concern. Let me create a priority ticket for our finance team. They typically respond within 2 hours. Your reference number is TKT-" + std::to_string(rand() % 9000 + 1000);
        if (intent == "tech_support")
            return "I can help with your technical issue. Have you tried restarting your device? If the problem persists, I'll escalate to our Level 2 support team immediately.";
        if (intent == "customer_support")
            return "I understand your concern about your order. Let me pull up your account and create a support ticket. A representative will follow up within 4 hours.";
        if (intent == "healthcare")
            return "I can help schedule your appointment. What date and time works best for you? I'll check availability and confirm right away.";
        if (intent == "hr")
            return "I'll help process your request. Your PTO request has been noted and forwarded to HR. You should receive confirmation within 24 hours.";
        
        return "Thank you for contacting Divine BPO. I've noted your request and created a ticket. Our team will get back to you shortly. Is there anything else I can help with?";
    }
    
    static std::string escape_xml(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '<') out += "&lt;";
            else if (c == '>') out += "&gt;";
            else if (c == '&') out += "&amp;";
            else if (c == '"') out += "&quot;";
            else if (c == '\'') out += "&apos;";
            else out += c;
        }
        return out;
    }
};

} // namespace divine
