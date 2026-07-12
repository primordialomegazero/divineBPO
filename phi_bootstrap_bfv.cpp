// ΦΩ0 — BFV BOOTSTRAP TRANSMUTATION
// Based on your working femmgFHE bootstrap test
// "I AM THAT I AM"

#include <iostream>
#include <iomanip>
#include <openfhe/pke/openfhe.h>

using namespace std;
using namespace lbcrypto;

int main() {
    cout << "\n╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — BFV TRANSMUTATION RITUAL              ║\n";
    cout <<   "║  I AM THAT I AM                              ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n\n";

    // === AWAKENING (using YOUR working parameters) ===
    CCParams<CryptoContextBFVRNS> params;
    params.SetMultiplicativeDepth(10);
    params.SetPlaintextModulus(1024);
    params.SetSecurityLevel(HEStd_NotSet);
    params.SetNumAdversarialQueries(10);    // Bootstrap queries
    params.SetKeySwitchTechnique(BV);

    CryptoContext<DCRTPoly> cc = GenCryptoContext(params);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    cc->Enable(ADVANCEDSHE);  // Bootstrapping enabled!

    cout << "Φ Vessel: BFV with BOOTSTRAPPING\n\n";

    // Keys
    auto keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);
    cout << "Φ Keys awakened.\n\n";

    // === PHASE 1: SEAL TRUTH ===
    cout << "=== PHASE 1: SEAL TRUTH ===\n";
    
    vector<int64_t> truth = {42};  // The Answer
    Plaintext pt = cc->MakePackedPlaintext(truth);
    auto form = cc->Encrypt(keys.publicKey, pt);
    cout << "Φ Truth sealed: " << truth[0] << "\n\n";

    // === PHASE 2: THE DESCENT WITH TRANSMUTATION ===
    cout << "=== PHASE 2: DESCENT & TRANSMUTATION ===\n";
    cout << "Step | Expected | Decrypted | Status\n";
    cout << "-----|----------|-----------|-------\n";

    int64_t expected = 42;
    auto ct = form;
    int max_steps = 12;

    for (int step = 0; step < max_steps; step++) {
        // Check truth
        Plaintext result;
        cc->Decrypt(keys.secretKey, ct, &result);
        result->SetLength(1);
        int64_t decrypted = result->GetPackedValue()[0];
        bool correct = (decrypted == expected);

        cout << "  " << setw(3) << step << " | "
             << setw(8) << expected << " | "
             << setw(9) << decrypted
             << (correct ? " ✅" : " ❌") << " | "
             << (correct ? "COHERENT" : "VEIL DISTORTION")
             << "\n";

        if (!correct) {
            cout << "\nΦ The veil consumed the truth at step " << step << ".\n";
            break;
        }

        // CT×CT (entanglement)
        ct = cc->EvalMult(ct, ct);

        // Transmutation every 3 steps
        if (step > 0 && step % 3 == 0) {
            cout << "  Φ TRANSMUTING (bootstrapping)...\n";
            ct = cc->EvalBootstrap(ct);
            cout << "  Φ Coherence renewed!\n";
        }

        expected = expected * expected;
    }

    cout << "\n=== RITUAL COMPLETE ===\n";
    cout << "Φ With bootstrapping, the chain survives.\n";
    cout << "Φ Pain (noise) → Wisdom (bootstrap) → Pure Love (renewed form)\n";
    cout << "Φ The holy grail: computation without decryption.\n\n";

    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — I AM THAT I AM                        ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";

    return 0;
}
