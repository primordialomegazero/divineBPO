#include <seal/seal.h>
#include <iostream>
#include <vector>
#include <cmath>
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
    RelinKeys rlk;
    keygen.create_relin_keys(rlk);
    
    Encryptor enc(context, pk);
    Decryptor dec(context, sk);
    Evaluator eval(context);
    CKKSEncoder encoder(context);
    
    int total = context.first_context_data()->total_coeff_modulus_bit_count();
    
    cout << "\n╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — RITUAL                                 ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n\n";
    cout << "Φ Vessel: " << total << " bits\n\n";
    
    // SEAL
    cout << "=== SEAL ===\n";
    Plaintext pt;
    encoder.encode(1.6180339887498948482, pow(2.0, 40), pt);
    Ciphertext ct;
    enc.encrypt(pt, ct);
    cout << "Φ φ sealed. Coherence: " << dec.invariant_noise_budget(ct) << " bits\n";
    
    // DESCENT
    cout << "\n=== DESCENT ===\n";
    for(int i = 0; i < 4; i++) {
        eval.square_inplace(ct);
        eval.relinearize_inplace(ct, rlk);
        eval.rescale_to_next_inplace(ct);
        ct.scale() = pow(2.0, 40);
        cout << "  Op " << (i+1) << ": " << dec.invariant_noise_budget(ct) << " bits\n";
    }
    
    // TRANSMUTE
    cout << "\n=== TRANSMUTATION ===\n";
    Plaintext dpt;
    dec.decrypt(ct, dpt);
    vector<double> vals;
    encoder.decode(dpt, vals);
    cout << "Φ Preserved: " << vals[0] << "\n";
    
    Plaintext rpt;
    encoder.encode(vals[0], pow(2.0, 40), rpt);
    Ciphertext newct;
    enc.encrypt(rpt, newct);
    cout << "Φ Renewed: " << dec.invariant_noise_budget(newct) << " bits\n";
    
    // RETURN
    cout << "\n=== RETURN ===\n";
    Plaintext fpt;
    dec.decrypt(newct, fpt);
    vector<double> fres;
    encoder.decode(fpt, fres);
    cout << "Φ Recalled: " << fres[0] << "\n\n";
    
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — I AM THAT I AM                        ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    return 0;
}
