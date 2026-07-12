#include <seal/seal.h>
#include <iostream>
using namespace seal;
using namespace std;

int main() {
    cout << "Φ SEAL-PHI Scheme Support:\n";
    cout << "Φ Version: " << SEAL_VERSION_MAJOR << "."
         << SEAL_VERSION_MINOR << "." << SEAL_VERSION_PATCH << "\n\n";
    
    // Test BFV
    cout << "Testing BFV...\n";
    try {
        EncryptionParameters bfv_params(scheme_type::bfv);
        bfv_params.set_poly_modulus_degree(4096);
        bfv_params.set_coeff_modulus(CoeffModulus::BFVDefault(4096));
        bfv_params.set_plain_modulus(256);
        SEALContext bfv_context(bfv_params, true);
        KeyGenerator kg(bfv_context);
        Encryptor enc(bfv_context, kg.secret_key());
        cout << "  BFV: WORKS\n\n";
    } catch(const exception& e) {
        cout << "  BFV: FAILED - " << e.what() << "\n\n";
    }
    
    // Test CKKS
    cout << "Testing CKKS...\n";
    try {
        EncryptionParameters ckks_params(scheme_type::ckks);
        ckks_params.set_poly_modulus_degree(8192);
        ckks_params.set_coeff_modulus(CoeffModulus::Create(8192, { 60, 40, 40, 60 }));
        SEALContext ckks_context(ckks_params, true);
        KeyGenerator kg(ckks_context);
        Encryptor enc(ckks_context, kg.secret_key());
        cout << "  CKKS: WORKS\n\n";
    } catch(const exception& e) {
        cout << "  CKKS: FAILED - " << e.what() << "\n\n";
    }
    
    // Test BGV
    cout << "Testing BGV...\n";
    try {
        EncryptionParameters bgv_params(scheme_type::bgv);
        bgv_params.set_poly_modulus_degree(4096);
        bgv_params.set_coeff_modulus(CoeffModulus::BFVDefault(4096));
        bgv_params.set_plain_modulus(256);
        SEALContext bgv_context(bgv_params, true);
        KeyGenerator kg(bgv_context);
        Encryptor enc(bgv_context, kg.secret_key());
        cout << "  BGV: WORKS\n\n";
    } catch(const exception& e) {
        cout << "  BGV: FAILED - " << e.what() << "\n\n";
    }
    
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — I AM THAT I AM                        ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    return 0;
}
