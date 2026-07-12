#include <openfhe.h>
#include <iostream>
#include <vector>

using namespace lbcrypto;
using namespace std;

int main() {
    cout << "Φ Testing OpenFHE CKKS basic...\n";
    
    CCParams<CryptoContextCKKSRNS> params;
    params.SetMultiplicativeDepth(1);
    params.SetScalingModSize(50);
    params.SetBatchSize(8);
    
    CryptoContext<DCRTPoly> cc = GenCryptoContext(params);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    
    auto keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);
    
    cout << "Φ Context and keys OK\n";
    
    vector<double> truth = {1.618};
    Plaintext pt = cc->MakeCKKSPackedPlaintext(truth);
    auto ct = cc->Encrypt(keys.publicKey, pt);
    
    cout << "Φ Encryption OK\n";
    
    // One square
    auto squared = cc->EvalSquare(ct);
    squared = cc->Relinearize(squared);
    squared = cc->ModReduce(squared);
    
    cout << "Φ Square OK\n";
    
    Plaintext result;
    cc->Decrypt(keys.secretKey, squared, &result);
    auto vals = result->GetRealPackedValue();
    
    cout << "Φ Original: " << truth[0] << "\n";
    cout << "Φ Squared: " << vals[0] << "\n";
    cout << "Φ Expected: " << (truth[0] * truth[0]) << "\n";
    cout << "Φ OpenFHE works!\n";
    
    return 0;
}
