// PHI-TWILIO — Voice/SMS Integration
// Twilio webhook handler + TwiML generation

#pragma once
#include <string>
#include <sstream>
#include <map>
#include <cstdio>

namespace divine {

class TwilioHandler {
private:
    std::string account_sid;
    std::string auth_token;
    std::string from_number;
    bool configured;

public:
    TwilioHandler() : configured(false) {}

    void configure(const std::string& sid, const std::string& token, const std::string& from) {
        account_sid = sid;
        auth_token = token;
        from_number = from;
        configured = true;
    }

    bool is_configured() const { return configured; }

    // Parse Twilio webhook request
    std::map<std::string, std::string> parse_webhook(const std::string& body) {
        std::map<std::string, std::string> params;
        std::istringstream stream(body);
        std::string pair;

        while (std::getline(stream, pair, '&')) {
            size_t eq = pair.find('=');
            if (eq != std::string::npos) {
                std::string key = pair.substr(0, eq);
                std::string value = pair.substr(eq + 1);
                params[key] = url_decode(value);
            }
        }
        return params;
    }

    // Generate TwiML for voice response
    std::string voice_response(const std::string& message, bool gather_input = true) {
        char buf[2048];
        if (gather_input) {
            snprintf(buf, sizeof(buf),
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                "<Response>"
                "<Say voice=\"Polly.Joanna\">%s</Say>"
                "<Gather input=\"speech dtmf\" timeout=\"5\" numDigits=\"1\" action=\"/api/twilio/voice/process\" method=\"POST\">"
                "<Say voice=\"Polly.Joanna\">Press 1 for billing, 2 for technical support, 3 for general inquiries, or simply speak your request.</Say>"
                "</Gather>"
                "<Say voice=\"Polly.Joanna\">I didn't receive your input. Goodbye.</Say>"
                "</Response>",
                escape_xml(message).c_str());
        } else {
            snprintf(buf, sizeof(buf),
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                "<Response>"
                "<Say voice=\"Polly.Joanna\">%s</Say>"
                "<Hangup/>"
                "</Response>",
                escape_xml(message).c_str());
        }
        return std::string(buf);
    }

    // Generate TwiML for SMS response
    std::string sms_response(const std::string& message) {
        char buf[2048];
        snprintf(buf, sizeof(buf),
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<Response>"
            "<Message>%s</Message>"
            "</Response>",
            escape_xml(message).c_str());
        return std::string(buf);
    }

private:
    static std::string url_decode(const std::string& s) {
        std::string result;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == '+') result += ' ';
            else if (s[i] == '%' && i + 2 < s.size()) {
                int hex;
                sscanf(s.substr(i + 1, 2).c_str(), "%x", &hex);
                result += (char)hex;
                i += 2;
            } else result += s[i];
        }
        return result;
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
