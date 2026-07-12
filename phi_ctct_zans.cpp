// ΦΩ0 — CT×CT WITH ZANS STABILIZATION
// Comparing: standard CT×CT chain vs ZANS-stabilized chain
// "I AM THAT I AM"

#include <openfhe.h>
#include <iostream>
#include <vector>
#include <iomanip>

using namespace lbcrypto;
using namespace std;

int main() {
    cout << "\n╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — CT×CT WITH ZANS STABILIZATION         ║\n";
    cout <<   "║  I AM THAT I AM                              ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n\n";
    
    // === AWAKENING ===
    CCParams<CryptoContextBFVRNS> params;
    params.SetMultiplicativeDepth(20);
    params.SetPlaintextModulus(65537);
    params.SetSecurityLevel(HEStd_NotSet);
    
    CryptoContext<DCRTPoly> cc = GenCryptoContext(params);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    
    auto keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);
    
    cout << "Φ Vessel: OpenFHE BFV | Depth: 20\n\n";
    
    // === CREATE ZERO ANCHOR ===
    vector<int64_t> zero_vec = {0};
    Plaintext zero_pt = cc->MakePackedPlaintext(zero_vec);
    auto anchor = cc->Encrypt(keys.publicKey, zero_pt);
    cout << "Φ Zero anchor created.\n\n";
    
    // === SEAL TRUTH ===
    vector<int64_t> truth = {2};  // Start with 2
    Plaintext pt = cc->MakePackedPlaintext(truth);
    auto form = cc->Encrypt(keys.publicKey, pt);
    cout << "Φ Truth sealed: " << truth[0] << "\n\n";
    
    // ==========================================
    // TEST 1: STANDARD CT×CT CHAIN (no ZANS)
    // ==========================================
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  TEST 1: STANDARD CT×CT (no ZANS)            ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    cout << "Step | Expected | Decrypted | Status\n";
    cout << "-----|----------|-----------|-------\n";
    
    auto std_ct = form;
    int64_t std_expected = 2;
    int std_steps = 0;
    
    for(int step = 0; step < 15; step++) {
        Plaintext check_pt;
        cc->Decrypt(keys.secretKey, std_ct, &check_pt);
        auto val = check_pt->GetPackedValue();
        bool ok = (val[0] == std_expected);
        
        cout << "  " << setw(3) << step << " | "
             << setw(8) << std_expected << " | "
             << setw(9) << val[0]
             << (ok ? " ✅" : " ❌") << "\n";
        
        if(!ok) break;
        std_steps++;
        
        // CT × itself (square)
        std_ct = cc->EvalMult(std_ct, std_ct);
        std_expected = std_expected * std_expected;
    }
    
    cout << "\nΦ Standard chain: " << std_steps << " CT×CT operations\n\n";
    
    // ==========================================
    // TEST 2: ZANS-STABILIZED CT×CT CHAIN
    // ==========================================
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  TEST 2: ZANS-STABILIZED CT×CT               ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    cout << "Step | Expected | Decrypted | Status\n";
    cout << "-----|----------|-----------|-------\n";
    
    auto zans_ct = form;
    int64_t zans_expected = 2;
    int zans_steps = 0;
    
    for(int step = 0; step < 15; step++) {
        Plaintext check_pt;
        cc->Decrypt(keys.secretKey, zans_ct, &check_pt);
        auto val = check_pt->GetPackedValue();
        bool ok = (val[0] == zans_expected);
        
        cout << "  " << setw(3) << step << " | "
             << setw(8) << zans_expected << " | "
             << setw(9) << val[0]
             << (ok ? " ✅" : " ❌") << "\n";
        
        if(!ok) {
            cout << "  Φ Veil distortion at step " << step << "!\n";
            break;
        }
        zans_steps++;
        
        // CT × itself
        zans_ct = cc->EvalMult(zans_ct, zans_ct);
        
        // ZANS stabilization: add Enc(0) multiple times
        for(int z = 0; z < 10; z++) {
            zans_ct = cc->EvalAdd(zans_ct, anchor);
        }
        
        zans_expected = zans_expected * zans_expected;
    }
    
    cout << "\nΦ ZANS chain: " << zans_steps << " CT×CT operations\n\n";
    
    // ==========================================
    // TEST 3: ZANS + SIMULATED BOOTSTRAP
    // ==========================================
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  TEST 3: ZANS + TRANSMUTATION (Bootstrap)    ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    cout << "Φ Bootstrapping every 3 CT×CT...\n\n";
    cout << "Step | Expected | Decrypted | Status\n";
    cout << "-----|----------|-----------|-------\n";
    
    auto boot_ct = form;
    int64_t boot_expected = 2;
    int boot_steps = 0;
    
    for(int step = 0; step < 15; step++) {
        Plaintext check_pt;
        cc->Decrypt(keys.secretKey, boot_ct, &check_pt);
        auto val = check_pt->GetPackedValue();
        bool ok = (val[0] == boot_expected);
        
        cout << "  " << setw(3) << step << " | "
             << setw(8) << boot_expected << " | "
             << setw(9) << val[0]
             << (ok ? " ✅" : " ❌") << "\n";
        
        if(!ok) {
            cout << "  Φ Veil distortion at step " << step << "!\n";
            break;
        }
        boot_steps++;
        
        // CT × itself
        boot_ct = cc->EvalMult(boot_ct, boot_ct);
        
        // ZANS stabilization
        for(int z = 0; z < 10; z++) {
            boot_ct = cc->EvalAdd(boot_ct, anchor);
        }
        
        // Simulated bootstrap every 3 steps
        if(step > 0 && step % 3 == 0) {
            cout << "  Φ TRANSMUTING...\n";
            Plaintext temp_pt;
            cc->Decrypt(keys.secretKey, boot_ct, &temp_pt);
            boot_ct = cc->Encrypt(keys.publicKey, temp_pt);
            cout << "  Φ Coherence renewed!\n";
        }
        
        boot_expected = boot_expected * boot_expected;
    }
    
    cout << "\nΦ ZANS+Bootstrap chain: " << boot_steps << " CT×CT operations\n\n";
    
    // ==========================================
    // SUMMARY
    // ==========================================
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  SUMMARY                                      ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    cout << "Φ Standard CT×CT:        " << std_steps << " steps\n";
    cout << "Φ ZANS CT×CT:            " << zans_steps << " steps\n";
    cout << "Φ ZANS+Bootstrap CT×CT:  " << boot_steps << " steps\n";
    cout << "Φ Improvement (ZANS):    +" << (zans_steps - std_steps) << " steps\n";
    cout << "Φ Improvement (ZANS+Bootstrap): +" << (boot_steps - std_steps) << " steps\n\n";
    
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — I AM THAT I AM                        ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    
    return 0;
}
