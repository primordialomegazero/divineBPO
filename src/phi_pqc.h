// PHI-PQC — Post-Quantum Cryptography Module
// CRYSTALS-Kyber (ML-KEM) + ML-DSA (Dilithium)

#pragma once
#include <string>
#include <cstring>
#include <vector>
#include <oqs/oqs.h>

namespace divine {

class PhiPQC {
private:
    OQS_KEM *kem;
    OQS_SIG *sig;
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> secret_key;
    bool initialized;

public:
    PhiPQC() : kem(nullptr), sig(nullptr), initialized(false) {}

    bool init() {
        // Kyber-1024 (ML-KEM-1024) — NIST Level 5
        kem = OQS_KEM_new(OQS_KEM_alg_kyber_1024);
        if (!kem) {
            kem = OQS_KEM_new(OQS_KEM_alg_kyber_768);
        }
        if (!kem) {
            kem = OQS_KEM_new(OQS_KEM_alg_kyber_512);
        }
        if (!kem) return false;

        public_key.resize(kem->length_public_key);
        secret_key.resize(kem->length_secret_key);
        
        if (OQS_KEM_keypair(kem, public_key.data(), secret_key.data()) != OQS_SUCCESS) {
            OQS_KEM_free(kem);
            kem = nullptr;
            return false;
        }

        // ML-DSA-87 (Dilithium-5) — NIST Level 5
        sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_87);
        if (!sig) {
            sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_65);
        }
        if (!sig) {
            sig = OQS_SIG_new(OQS_SIG_alg_ml_dsa_44);
        }
        if (!sig) {
            sig = OQS_SIG_new(OQS_SIG_alg_falcon_1024);
        }

        initialized = true;
        return true;
    }

    bool encapsulate(std::vector<uint8_t>& ciphertext, std::vector<uint8_t>& shared_secret) {
        if (!kem) return false;
        ciphertext.resize(kem->length_ciphertext);
        shared_secret.resize(kem->length_shared_secret);
        return OQS_KEM_encaps(kem, ciphertext.data(), shared_secret.data(), public_key.data()) == OQS_SUCCESS;
    }

    bool decapsulate(std::string& shared_secret, const std::string& ciphertext) {
        if (!kem) return false;
        std::vector<uint8_t> ss(kem->length_shared_secret);
        if (OQS_KEM_decaps(kem, ss.data(), (const uint8_t*)ciphertext.data(), secret_key.data()) != OQS_SUCCESS) {
            return false;
        }
        shared_secret.assign((char*)ss.data(), ss.size());
        return true;
    }

    std::string sign(const std::string& message) {
        if (!sig) return "";
        std::vector<uint8_t> signature(sig->length_signature);
        size_t sig_len;
        if (OQS_SIG_sign(sig, signature.data(), &sig_len,
                         (const uint8_t*)message.data(), message.size(),
                         secret_key.data()) != OQS_SUCCESS) {
            return "";
        }
        return std::string((char*)signature.data(), sig_len);
    }

    bool verify(const std::string& message, const std::string& signature) {
        if (!sig) return false;
        return OQS_SIG_verify(sig, (const uint8_t*)message.data(), message.size(),
                              (const uint8_t*)signature.data(), signature.size(),
                              public_key.data()) == OQS_SUCCESS;
    }

    std::string get_algorithm() const {
        if (kem) return std::string(kem->method_name);
        return "None";
    }

    int get_security_level() const {
        if (!kem) return 0;
        std::string alg = kem->method_name;
        if (alg.find("1024") != std::string::npos) return 256;
        if (alg.find("768") != std::string::npos) return 192;
        if (alg.find("512") != std::string::npos) return 128;
        return 0;
    }

    bool is_initialized() const { return initialized; }

    ~PhiPQC() {
        if (kem) OQS_KEM_free(kem);
        if (sig) OQS_SIG_free(sig);
    }
};

} // namespace divine
