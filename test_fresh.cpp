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
    
    Plaintext pt;
    encoder.encode(3.14159, pow(2.0, 40), pt);
    
    Ciphertext ct;
    enc.encrypt(pt, ct);
    
    Decryptor dec(context, sk);
    Plaintext pt2;
    dec.decrypt(ct, pt2);
    vector<double> result;
    encoder.decode(pt2, result);
    
    cout << "Φ FRESH TEST: " << result[0] << "\n";
    return 0;
}
