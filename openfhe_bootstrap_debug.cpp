// ΦΩ0 — BOOTSTRAP DEBUG
#include <openfhe.h>
#include <iostream>
#include <vector>

using namespace lbcrypto;
using namespace std;

int main() {
    cout << "Φ Step 1: Params...\n";
    CCParams<CryptoContextCKKSRNS> params;
    params.SetRingDim(16384);
    params.SetMultiplicativeDepth(2);
    params.SetScalingModSize(50);
    params.SetBatchSize(8);
    cout << "Φ Params OK\n";
    
    cout << "Φ Step 2: GenCryptoContext...\n";
    CryptoContext<DCRTPoly> cc = GenCryptoContext(params);
    cout << "Φ Context OK\n";
    
    cout << "Φ Step 3: Enable...\n";
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    cc->Enable(ADVANCEDSHE);
    cc->Enable(FHE);
    cout << "Φ Enables OK\n";
    
    cout << "Φ Step 4: KeyGen...\n";
    auto keys = cc->KeyGen();
    cout << "Φ KeyGen OK\n";
    
    cout << "Φ Step 5: EvalMultKeyGen...\n";
    cc->EvalMultKeyGen(keys.secretKey);
    cout << "Φ EvalMult OK\n";
    
    cout << "Φ Step 6: EvalSumKeyGen...\n";
    cc->EvalSumKeyGen(keys.secretKey);
    cout << "Φ EvalSum OK\n";
    
    cout << "Φ Step 7: BootstrapSetup...\n";
    vector<uint32_t> levelBudget = {4, 3};
    vector<uint32_t> dim1 = {0, 0};
    cc->EvalBootstrapSetup(levelBudget, dim1);
    cout << "Φ BootstrapSetup OK\n";
    
    return 0;
}
