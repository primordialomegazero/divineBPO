// ΦΩ0 — OPENFHE BOOTSTRAP v3.0
// All enables: PKE + KEYSWITCH + LEVELEDSHE + ADVANCEDSHE + FHE
#include <openfhe.h>
#include <iostream>
#include <vector>

using namespace lbcrypto;
using namespace std;

int main() {
    cout << "\n╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — BOOTSTRAP v3.0                         ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n\n";
    
    CCParams<CryptoContextCKKSRNS> params;
    params.SetRingDim(16384);
    params.SetMultiplicativeDepth(2);
    params.SetScalingModSize(50);
    params.SetBatchSize(8);
    
    CryptoContext<DCRTPoly> cc = GenCryptoContext(params);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    cc->Enable(ADVANCEDSHE);
    cc->Enable(FHE);
    
    cout << "Φ Ring: " << cc->GetRingDimension() << "\n";
    cout << "Φ PKE LEVELEDSHE ADVANCEDSHE FHE: ALL ENABLED\n\n";
    
    auto keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);
    cc->EvalSumKeyGen(keys.secretKey);
    
    vector<uint32_t> levelBudget = {4, 3};
    vector<uint32_t> dim1 = {0, 0};
    
    cout << "Φ Generating bootstrap keys...\n";
    cc->EvalBootstrapSetup(levelBudget, dim1);
    cout << "Φ Bootstrap keys ready.\n\n";
    
    // Seal φ
    vector<double> truth = {1.6180339887498948482};
    Plaintext pt = cc->MakeCKKSPackedPlaintext(truth);
    auto ct = cc->Encrypt(keys.publicKey, pt);
    cout << "Φ φ sealed.\n\n";
    
    // Descent: 1 operation
    cout << "=== DESCENT ===\n";
    auto degraded = cc->EvalSquare(ct);
    degraded = cc->Relinearize(degraded);
    degraded = cc->ModReduce(degraded);
    cout << "Φ 1 op complete.\n\n";
    
    // Transmutation
    cout << "=== TRANSMUTATION ===\n";
    auto renewed = cc->EvalBootstrap(degraded);
    cout << "Φ BOOTSTRAP COMPLETE!\n\n";
    
    // Return
    cout << "=== RETURN ===\n";
    Plaintext result_pt;
    cc->Decrypt(keys.secretKey, renewed, &result_pt);
    auto final_truth = result_pt->GetRealPackedValue();
    
    cout << "Φ Original: " << truth[0] << "\n";
    cout << "Φ Recalled: " << final_truth[0] << "\n\n";
    
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — I AM THAT I AM                        ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    
    return 0;
}
