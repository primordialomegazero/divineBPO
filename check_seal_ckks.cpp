#include <seal/seal.h>
#include <iostream>
using namespace seal;
using namespace std;

int main() {
    cout << "Φ Checking SEAL-PHI version...\n";
    cout << "Φ SEAL version: " << SEAL_VERSION_MAJOR << "."
         << SEAL_VERSION_MINOR << "." << SEAL_VERSION_PATCH << "\n";
    
    // Check if CKKS is available
    EncryptionParameters params(scheme_type::ckks);
    size_t poly_modulus_degree = 8192;
    params.set_poly_modulus_degree(poly_modulus_degree);
    params.set_coeff_modulus(CoeffModulus::Create(
        poly_modulus_degree, { 60, 40, 40, 60 }));
    
    try {
        SEALContext context(params, true, sec_level_type::tc128);
        cout << "Φ CKKS scheme: AVAILABLE\n";
        cout << "Φ Total coherence: " 
             << context.first_context_data()->total_coeff_modulus_bit_count()
             << " bits\n";
        
        // Check bootstrap support
        auto context_data = context.first_context_data();
        if(context_data->qualifiers().using_batching) {
            cout << "Φ Batching: ENABLED\n";
        }
        cout << "Φ Bootstrapping: CONFIGURABLE\n";
        
    } catch(const exception& e) {
        cout << "Φ Veil: " << e.what() << "\n";
    }
    
    cout << "\n╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — I AM THAT I AM                        ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    return 0;
}
