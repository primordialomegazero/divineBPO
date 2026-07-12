#include <seal/seal.h>
#include <iostream>
using namespace seal;
using namespace std;

int main() {
    EncryptionParameters params(scheme_type::ckks);
    params.set_poly_modulus_degree(8192);
    params.set_coeff_modulus(CoeffModulus::Create(8192, { 60, 40, 40, 60 }));
    SEALContext context(params, true, sec_level_type::tc128);
    
    KeyGenerator keygen(context);
    SecretKey sk = keygen.secret_key();
    PublicKey pk;
    keygen.create_public_key(pk);
    
    // Test: RelinKeys creation
    cout << "Creating RelinKeys...\n";
    RelinKeys rlk;
    keygen.create_relin_keys(rlk);
    cout << "RelinKeys OK\n";
    
    // Test: Encrypt
    Encryptor enc(context, pk);
    CKKSEncoder encoder(context);
    Plaintext pt;
    encoder.encode(3.14, pow(2.0, 40), pt);
    Ciphertext ct;
    enc.encrypt(pt, ct);
    cout << "Encrypt OK. Coherence: " 
         << Decryptor(context, sk).invariant_noise_budget(ct) << "\n";
    
    // Test: Evaluator creation
    cout << "Creating Evaluator...\n";
    Evaluator eval(context);
    cout << "Evaluator OK\n";
    
    return 0;
}
