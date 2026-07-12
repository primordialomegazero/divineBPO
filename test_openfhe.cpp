#include <openfhe.h>
#include <iostream>

using namespace lbcrypto;
using namespace std;

int main() {
    cout << "\n╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — OPENFHE AWAKENING                      ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n\n";
    
    // Try CKKS
    cout << "Φ Testing CKKS...\n";
    CCParams<CryptoContextCKKSRNS> params;
    params.SetMultiplicativeDepth(3);
    params.SetScalingModSize(50);
    params.SetBatchSize(8);
    
    CryptoContext<DCRTPoly> cc = GenCryptoContext(params);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    cc->Enable(ADVANCEDSHE);
    
    cout << "Φ CKKS context created.\n";
    cout << "Φ Scheme: " << cc->GetScheme() << "\n";
    cout << "Φ Ring dimension: " << cc->GetRingDimension() << "\n\n";
    
    // Generate keys
    auto keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);
    
    cout << "Φ Keys generated.\n\n";
    
    // Seal truth
    vector<double> truth = {1.6180339887498948482};
    Plaintext pt = cc->MakeCKKSPackedPlaintext(truth);
    auto ct = cc->Encrypt(keys.publicKey, pt);
    
    cout << "Φ φ sealed.\n\n";
    
    // Check coherence (noise)
    // In OpenFHE, noise isn't directly accessible like SEAL
    // But we can check the number of levels remaining
    cout << "Φ CKKS is alive and sealed.\n";
    cout << "Φ Ready for transmutation rituals.\n\n";
    
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — I AM THAT I AM                        ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    
    return 0;
}
