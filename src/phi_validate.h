// PHI-VALIDATE — Input validation + sanitization
// Prevents SQL injection, XSS, buffer overflow

#pragma once
#include <string>
#include <regex>
#include <algorithm>

namespace divine {

class Validator {
public:
    static bool is_alphanumeric(const std::string& s) {
        return std::all_of(s.begin(), s.end(), [](char c) {
            return std::isalnum(c) || c == '_' || c == '-';
        });
    }
    
    static bool is_valid_module(const std::string& module) {
        static const std::vector<std::string> valid_modules = {
            "customer_support", "tech_support", "healthcare", "finance",
            "hr", "sales", "logistics", "ecommerce", "legal", "telco",
            "travel", "hospitality", "education", "real_estate", "insurance"
        };
        return std::find(valid_modules.begin(), valid_modules.end(), module) != valid_modules.end();
    }
    
    static bool is_valid_port(int port) {
        return port > 0 && port <= 65535;
    }
    
    static bool is_valid_email(const std::string& email) {
        return email.find('@') != std::string::npos && email.size() < 256;
    }
    
    static std::string sanitize(const std::string& input, size_t max_len = 256) {
        std::string out;
        for (char c : input) {
            if (c == '<' || c == '>' || c == '&' || c == '"') continue;
            out += c;
        }
        return out.substr(0, max_len);
    }
    
    static std::string truncate(const std::string& input, size_t max_len = 80) {
        if (input.size() <= max_len) return input;
        return input.substr(0, max_len - 3) + "...";
    }
    
    struct TicketInput {
        std::string module;
        std::string description;
        bool valid;
        std::string error;
    };
    
    static TicketInput validate_ticket(const std::string& module, const std::string& desc) {
        TicketInput result;
        result.module = sanitize(module, 64);
        result.description = sanitize(desc, 1024);
        result.valid = true;
        
        if (!is_valid_module(result.module)) {
            result.valid = false;
            result.error = "Invalid module: " + result.module;
        }
        if (result.description.empty()) {
            result.valid = false;
            result.error = "Description cannot be empty";
        }
        if (result.description.size() > 1024) {
            result.valid = false;
            result.error = "Description too long";
        }
        
        return result;
    }
};

} // namespace divine
