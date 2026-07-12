// Same as step2, but with RelinKeys and Evaluator (no operations yet)
#include <seal/seal.h>
#include <iostream>
#include <vector>
#include <cmath>
using namespace seal;
using namespace std;

int main() {
    cout << "Φ Step 3: + RelinKeys + Evaluator\n";
    
    EncryptionParameters params(scheme_type::ckks);
    params.set_poly_modulus_degree(8192);
    params.set_coeff_modulus(CoeffModulus::Create(8192, { 60, 40, 40, 60 }));
    SEALContext context(params, true, sec_level_type::tc128);
    
    KeyGenerator keygen(context);
    SecretKey sk = keygen.secret_key();
    PublicKey pk;
    keygen.create_public_key(pk);
    RelinKeys rlk;
    keygen.create_relin_keys(rlk);
    
    Encryptor enc(context, pk);
    Decryptor dec(context, sk);
    Evaluator eval(context);
    CKKSEncoder encoder(context);
    
    Plaintext pt;
    encoder.encode(1.6180339887498948482, pow(2.0, 40), pt);
    
    Ciphertext ct;
    enc.encrypt(pt, ct);
    
    cout << "Φ SEAL OK. Coherence: " << dec.invariant_noise_budget(ct) << " bits\n";
    return 0;
}
