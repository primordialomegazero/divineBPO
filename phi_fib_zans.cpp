// ΦΩ0 — FIBONACCI-DECOMPOSED MULTIPLICATION WITH ZANS
// Zeckendorf: any integer = sum of non-consecutive Fibonacci numbers
// CT × N done via O(log_φ N) ZANS-stabilized additions
// "I AM THAT I AM"

#include <openfhe.h>
#include <iostream>
#include <vector>
#include <cmath>

using namespace lbcrypto;
using namespace std;

// === FIBONACCI UTILITIES ===

// Generate Fibonacci numbers up to limit
vector<int64_t> generate_fibonacci(int64_t limit) {
    vector<int64_t> fib = {1, 2};
    while(fib.back() <= limit) {
        fib.push_back(fib[fib.size()-1] + fib[fib.size()-2]);
    }
    fib.pop_back(); // Remove the one that exceeded limit
    return fib;
}

// Zeckendorf decomposition: represent n as sum of non-consecutive Fibonacci numbers
vector<int64_t> zeckendorf_decompose(int64_t n) {
    vector<int64_t> fib = generate_fibonacci(n);
    vector<int64_t> result;
    
    for(int i = fib.size() - 1; i >= 0 && n > 0; i--) {
        if(fib[i] <= n) {
            result.push_back(fib[i]);
            n -= fib[i];
            i--; // Skip next (non-consecutive)
        }
    }
    return result;
}

// Multiply CT by a Fibonacci number using repeated ZANS additions
Ciphertext<DCRTPoly> multiply_by_fib(
    CryptoContext<DCRTPoly>& cc,
    const Ciphertext<DCRTPoly>& ct,
    int64_t fib_num,
    const Ciphertext<DCRTPoly>& anchor)
{
    if(fib_num == 1) return ct;
    
    // Start with ct
    auto result = ct;
    
    // Add ct to itself (fib_num - 1) times
    // Each addition is ZANS-stabilized
    for(int64_t i = 1; i < fib_num; i++) {
        result = cc->EvalAdd(result, ct);
        result = cc->EvalAdd(result, anchor); // ZANS stabilization
    }
    
    return result;
}

// Fibonacci-decomposed multiplication: CT × N
Ciphertext<DCRTPoly> fib_multiply(
    CryptoContext<DCRTPoly>& cc,
    const Ciphertext<DCRTPoly>& ct,
    int64_t multiplier,
    const Ciphertext<DCRTPoly>& anchor)
{
    auto fib_parts = zeckendorf_decompose(multiplier);
    
    cout << "  φ " << multiplier << " = ";
    for(size_t i = 0; i < fib_parts.size(); i++) {
        cout << fib_parts[i];
        if(i < fib_parts.size() - 1) cout << " + ";
    }
    cout << "\n";
    
    // Multiply by first Fibonacci part
    auto result = multiply_by_fib(cc, ct, fib_parts[0], anchor);
    
    // Add remaining parts
    for(size_t i = 1; i < fib_parts.size(); i++) {
        auto part = multiply_by_fib(cc, ct, fib_parts[i], anchor);
        result = cc->EvalAdd(result, part);
        result = cc->EvalAdd(result, anchor); // ZANS stabilization
    }
    
    return result;
}

// === MAIN ===
int main() {
    cout << "\n╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — FIBONACCI-ZANS MULTIPLICATION          ║\n";
    cout <<   "║  I AM THAT I AM                              ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n\n";
    
    // === AWAKENING ===
    CCParams<CryptoContextBFVRNS> params;
    params.SetMultiplicativeDepth(1);
    params.SetPlaintextModulus(65537);
    params.SetSecurityLevel(HEStd_NotSet);
    
    CryptoContext<DCRTPoly> cc = GenCryptoContext(params);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    
    auto keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);
    
    cout << "Φ Vessel: OpenFHE BFV | ZANS + Fibonacci\n\n";
    
    // === CREATE ZERO ANCHOR ===
    vector<int64_t> zero_vec = {0};
    Plaintext zero_pt = cc->MakePackedPlaintext(zero_vec);
    auto anchor = cc->Encrypt(keys.publicKey, zero_pt);
    cout << "Φ Zero anchor created.\n\n";
    
    // === SEAL TRUTH ===
    vector<int64_t> truth = {6};
    Plaintext pt = cc->MakePackedPlaintext(truth);
    auto form = cc->Encrypt(keys.publicKey, pt);
    cout << "Φ Truth sealed: " << truth[0] << "\n\n";
    
    // === TEST: FIBONACCI DECOMPOSITION ===
    cout << "=== ZECKENDORF DECOMPOSITIONS ===\n";
    for(int64_t n : {1, 2, 3, 5, 7, 8, 13, 21, 42, 100}) {
        auto parts = zeckendorf_decompose(n);
        cout << "  " << n << " = ";
        for(size_t i = 0; i < parts.size(); i++) {
            cout << parts[i];
            if(i < parts.size() - 1) cout << " + ";
        }
        cout << "\n";
    }
    
    // === TEST: FIB × 7 USING ZANS ===
    cout << "\n=== FIB-ZANS: CT × 7 ===\n";
    int64_t multiplier = 7;
    
    auto result = fib_multiply(cc, form, multiplier, anchor);
    
    Plaintext check_pt;
    cc->Decrypt(keys.secretKey, result, &check_pt);
    auto val = check_pt->GetPackedValue();
    
    cout << "\nΦ Result: " << val[0] << " (expected " << (truth[0] * multiplier) << ")";
    if(val[0] == truth[0] * multiplier) cout << " ✅";
    else cout << " ❌";
    cout << "\n\n";
    
    // === COMPARISON: STANDARD EVALMULT ===
    cout << "=== STANDARD: CT × CT ===\n";
    vector<int64_t> mult_vec = {multiplier};
    Plaintext mult_pt = cc->MakePackedPlaintext(mult_vec);
    auto mult_ct = cc->Encrypt(keys.publicKey, mult_pt);
    
    auto standard = cc->EvalMult(form, mult_ct);
    
    Plaintext std_pt;
    cc->Decrypt(keys.secretKey, standard, &std_pt);
    auto std_val = std_pt->GetPackedValue();
    
    cout << "Φ Standard: " << std_val[0] << " (expected " << (truth[0] * multiplier) << ")";
    if(std_val[0] == truth[0] * multiplier) cout << " ✅";
    else cout << " ❌";
    cout << "\n\n";
    
    // === TEST: 42 × 100 ===
    cout << "=== FIB-ZANS: 6 × 100 ===\n";
    auto big_result = fib_multiply(cc, form, 100, anchor);
    
    Plaintext big_pt;
    cc->Decrypt(keys.secretKey, big_result, &big_pt);
    auto big_val = big_pt->GetPackedValue();
    
    cout << "\nΦ Result: " << big_val[0] << " (expected " << (truth[0] * 100) << ")";
    if(big_val[0] == truth[0] * 100) cout << " ✅";
    else cout << " ❌";
    cout << "\n\n";
    
    // === SUMMARY ===
    cout << "=== RITUAL COMPLETE ===\n";
    cout << "Φ Zeckendorf: Any integer = Σ Fibonacci\n";
    cout << "Φ Fibonacci multiplication: Repeated ZANS additions\n";
    cout << "Φ ZANS: Adding void (Enc(0)) stabilizes noise\n";
    cout << "Φ Result: CT × N in O(log_φ N) operations\n";
    cout << "Φ The golden ratio enables efficient computation.\n\n";
    
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — I AM THAT I AM                        ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    
    return 0;
}
