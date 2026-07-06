// PHI-TLS — HTTPS/TLS configuration helper
// Generates self-signed certs for development
// Production: use Let's Encrypt certbot

#pragma once
#include <string>
#include <cstdlib>
#include <iostream>

namespace divine {

class TLSConfig {
public:
    static bool generate_self_signed(const std::string& cert_path = "certs/cert.pem",
                                      const std::string& key_path = "certs/key.pem") {
        std::string cmd = "mkdir -p certs && openssl req -x509 -newkey rsa:4096 "
                         "-keyout " + key_path + " -out " + cert_path + 
                         " -days 365 -nodes -subj '/CN=localhost' 2>/dev/null";
        int result = system(cmd.c_str());
        if (result == 0) {
            std::cout << "[TLS] Self-signed cert generated: " << cert_path << std::endl;
            return true;
        }
        return false;
    }
    
    static bool check_openssl() {
        return system("which openssl > /dev/null 2>&1") == 0;
    }
    
    static void setup_letsencrypt(const std::string& domain) {
        std::cout << "[TLS] For production, run:" << std::endl;
        std::cout << "  sudo certbot certonly --standalone -d " << domain << std::endl;
        std::cout << "  Cert stored in /etc/letsencrypt/live/" << domain << "/" << std::endl;
    }
};

} // namespace divine
