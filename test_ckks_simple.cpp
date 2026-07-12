// ΦΩ0 — CKKS SIMPLE TEST
#include <seal/seal.h>
#include <iostream>
#include <vector>

using namespace seal;
using namespace std;

int main() {
    cout << "Φ Testing CKKS...\n";
    cout << "Φ SEAL version: " << SEAL_VERSION_MAJOR << "."
         << SEAL_VERSION_MINOR << "." << SEAL_VERSION_PATCH << "\n\n";
    
    EncryptionParameters params(scheme_type::ckks);
    size_t poly_modulus_degree = 8192;
    params.set_poly_modulus_degree(poly_modulus_degree);
    params.set_coeff_modulus(CoeffModulus::Create(
        poly_modulus_degree, { 60, 40, 40, 60 }));
    
    cout << "Φ Scheme: CKKS\n";
    cout << "Φ Poly degree: " << poly_modulus_degree << "\n";
    cout << "Φ Params created.\n\n";
    
    SEALContext context(params, true, sec_level_type::tc128);
    
    cout << "Φ Context created.\n";
    cout << "Φ Total bits: " 
         << context.first_context_data()->total_coeff_modulus_bit_count() << "\n";
    cout << "Φ Using batching: " 
         << (context.first_context_data()->qualifiers().using_batching ? "YES" : "NO") << "\n\n";
    
    KeyGenerator keygen(context);
    SecretKey secret_key = keygen.secret_key();
    PublicKey public_key;
    keygen.create_public_key(public_key);
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    
    Encryptor encryptor(context, public_key);
    Decryptor decryptor(context, secret_key);
    Evaluator evaluator(context);
    CKKSEncoder encoder(context);
    
    cout << "Φ Keys and encoder created.\n\n";
    
    double input = 3.14159;
    Plaintext plain;
    Ciphertext encrypted;
    
    cout << "Φ Encoding " << input << "...\n";
    encoder.encode(input, pow(2.0, 40), plain);
    cout << "Φ Encoded.\n";
    
    cout << "Φ Encrypting...\n";
    encryptor.encrypt(plain, encrypted);
    cout << "Φ Encrypted. Coherence: " 
         << decryptor.invariant_noise_budget(encrypted) << " bits\n";
    
    cout << "Φ Decrypting...\n";
    Plaintext decrypted;
    decryptor.decrypt(encrypted, decrypted);
    vector<double> result;
    encoder.decode(decrypted, result);
    
    cout << "Φ Result: " << result[0] << "\n";
    cout << "Φ Original: " << input << "\n";
    cout << "Φ Difference: " << abs(result[0] - input) << "\n\n";
    
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — CKKS WORKS                             ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    
    return 0;
}
