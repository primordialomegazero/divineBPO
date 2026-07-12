#include <seal/seal.h>
#include <iostream>
#include <vector>
using namespace seal;
using namespace std;

int main() {
    cout << "Φ Step 1: Params...\n";
    EncryptionParameters params(scheme_type::ckks);
    params.set_poly_modulus_degree(8192);
    params.set_coeff_modulus(CoeffModulus::Create(8192, { 60, 40, 40, 60 }));
    
    cout << "Φ Step 2: Context...\n";
    SEALContext context(params, true, sec_level_type::tc128);
    
    cout << "Φ Step 3: KeyGenerator...\n";
    KeyGenerator keygen(context);
    
    cout << "Φ Step 4: SecretKey...\n";
    SecretKey sk = keygen.secret_key();
    
    cout << "Φ Step 5: PublicKey...\n";
    PublicKey pk;
    keygen.create_public_key(pk);
    
    cout << "Φ Step 6: Encryptor...\n";
    Encryptor enc(context, pk);
    
    cout << "Φ Step 7: Encoder...\n";
    CKKSEncoder encoder(context);
    
    cout << "Φ Step 8: Encode...\n";
    Plaintext pt;
    encoder.encode(3.14159, pow(2.0, 40), pt);
    
    cout << "Φ Step 9: Encrypt...\n";
    Ciphertext ct;
    enc.encrypt(pt, ct);
    
    cout << "Φ Step 10: Decrypt...\n";
    Decryptor dec(context, sk);
    Plaintext pt2;
    dec.decrypt(ct, pt2);
    
    cout << "Φ Step 11: Decode...\n";
    vector<double> result;
    encoder.decode(pt2, result);
    
    cout << "Φ Result: " << result[0] << "\n";
    cout << "Φ SUCCESS\n";
    return 0;
}
