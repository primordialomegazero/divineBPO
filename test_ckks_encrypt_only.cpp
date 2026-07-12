#include <seal/seal.h>
#include <iostream>
#include <vector>
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
    
    Encryptor enc(context, pk);
    CKKSEncoder encoder(context);
    
    double truth = 1.6180339887498948482;
    Plaintext pt;
    encoder.encode(truth, pow(2.0, 40), pt);
    
    cout << "About to encrypt...\n";
    Ciphertext ct;
    enc.encrypt(pt, ct);
    cout << "Encryption done. Coherence: " 
         << Decryptor(context, sk).invariant_noise_budget(ct) << "\n";
    return 0;
}
