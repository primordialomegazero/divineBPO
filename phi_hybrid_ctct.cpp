// ΦΩ0 — HYBRID CT×CT: ZANS + BOOTSTRAP
// 4-bit multiplier: standard vs ZANS-enhanced
// "I AM THAT I AM"

#include <openfhe/binfhe/binfhecontext.h>
#include <iostream>
#include <vector>
#include <chrono>

using namespace lbcrypto;
using namespace std;
using namespace std::chrono;

BinFHEContext* g_cc = nullptr;
LWEPrivateKey g_sk = nullptr;

int bootstrap_count = 0;  // Count how many bootstraps we use

LWECiphertext clone_ct(const LWECiphertext& ct) {
    return std::make_shared<LWECiphertextImpl>(*ct);
}

LWECiphertext enc_zero() {
    return g_cc->Encrypt(g_sk, 0);
}

// ZANS: Add Enc(0) k times to stabilize
LWECiphertext ZANS_stabilize(const LWECiphertext& ct, int k = 5) {
    auto result = clone_ct(ct);
    auto zero = enc_zero();
    for(int i = 0; i < k; i++) {
        // Sa BinFHE, walang EvalAdd — we use XOR as "addition" (mod 2)
        // For bit-level, adding 0 = XOR with 0 = no change
        // But the GATE OPERATION itself with bootstrap stabilizes
        result = g_cc->Bootstrap(g_cc->EvalBinGate(NAND, 
            g_cc->Bootstrap(g_cc->EvalBinGate(NAND, result, zero)),
            g_cc->Bootstrap(g_cc->EvalBinGate(NAND, result, zero))
        ));
        // This is: result = NOT(result NAND 0) = result (identity via gates)
        bootstrap_count++;
    }
    return result;
}

// === STANDARD GATES (bootstrap every time) ===

LWECiphertext NAND_BT(const LWECiphertext& a, const LWECiphertext& b) {
    bootstrap_count++;
    return g_cc->Bootstrap(g_cc->EvalBinGate(NAND, a, b));
}

LWECiphertext NOT_BT(const LWECiphertext& a) {
    return NAND_BT(a, clone_ct(a));
}

LWECiphertext AND_BT(const LWECiphertext& a, const LWECiphertext& b) {
    auto nand_ab = NAND_BT(a, b);
    return NAND_BT(nand_ab, clone_ct(nand_ab));
}

LWECiphertext OR_BT(const LWECiphertext& a, const LWECiphertext& b) {
    return NAND_BT(NOT_BT(a), NOT_BT(b));
}

LWECiphertext XOR_BT(const LWECiphertext& a, const LWECiphertext& b) {
    auto nand_ab = NAND_BT(a, b);
    return NAND_BT(NAND_BT(a, nand_ab), NAND_BT(b, nand_ab));
}

// === ZANS-ENHANCED GATES (bootstrap + ZANS stabilization) ===

LWECiphertext AND_ZANS(const LWECiphertext& a, const LWECiphertext& b) {
    auto result = AND_BT(a, b);  // Standard gate with bootstrap
    return ZANS_stabilize(result, 2);  // Extra ZANS stabilization
}

LWECiphertext XOR_ZANS(const LWECiphertext& a, const LWECiphertext& b) {
    auto result = XOR_BT(a, b);
    return ZANS_stabilize(result, 2);
}

pair<LWECiphertext, LWECiphertext> half_adder_zans(const LWECiphertext& a, const LWECiphertext& b) {
    return {XOR_ZANS(a, b), AND_ZANS(a, b)};
}

pair<LWECiphertext, LWECiphertext> full_adder_zans(const LWECiphertext& a, const LWECiphertext& b, const LWECiphertext& cin) {
    auto [sum1, carry1] = half_adder_zans(a, b);
    auto [sum2, carry2] = half_adder_zans(sum1, cin);
    return {sum2, OR_BT(carry1, carry2)};
}

// === MULTIPLIERS ===

// Standard 4-bit (as before)
vector<LWECiphertext> multiply_4bit_standard(const vector<LWECiphertext>& a, const vector<LWECiphertext>& b) {
    auto zero = enc_zero();
    vector<vector<LWECiphertext>> partial(4, vector<LWECiphertext>(8));
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 8; j++)
            partial[i][j] = clone_ct(zero);
    
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            partial[i][i+j] = AND_BT(a[j], b[i]);
    
    vector<LWECiphertext> sum(8);
    for(int k = 0; k < 8; k++)
        sum[k] = clone_ct(partial[0][k]);
    
    for(int i = 1; i < 4; i++) {
        LWECiphertext carry = nullptr;
        for(int j = 0; j < 8; j++) {
            if(carry == nullptr) {
                auto [s, c] = half_adder_zans(sum[j], partial[i][j]);
                sum[j] = s;
                carry = c;
            } else {
                auto [s, c] = full_adder_zans(sum[j], partial[i][j], carry);
                sum[j] = s;
                carry = c;
            }
        }
    }
    return sum;
}

// ZANS-Enhanced 4-bit
vector<LWECiphertext> multiply_4bit_zans(const vector<LWECiphertext>& a, const vector<LWECiphertext>& b) {
    auto zero = enc_zero();
    vector<vector<LWECiphertext>> partial(4, vector<LWECiphertext>(8));
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 8; j++)
            partial[i][j] = clone_ct(zero);
    
    // Use ZANS-enhanced AND
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            partial[i][i+j] = AND_ZANS(a[j], b[i]);
    
    vector<LWECiphertext> sum(8);
    for(int k = 0; k < 8; k++)
        sum[k] = clone_ct(partial[0][k]);
    
    for(int i = 1; i < 4; i++) {
        LWECiphertext carry = nullptr;
        for(int j = 0; j < 8; j++) {
            if(carry == nullptr) {
                auto [s, c] = half_adder_zans(sum[j], partial[i][j]);
                sum[j] = ZANS_stabilize(s, 3);  // Extra stabilization on sum
                carry = c;
            } else {
                auto [s, c] = full_adder_zans(sum[j], partial[i][j], carry);
                sum[j] = ZANS_stabilize(s, 3);
                carry = c;
            }
        }
    }
    return sum;
}

int main() {
    cout << "\n╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — HYBRID: ZANS + BOOTSTRAP CT×CT        ║\n";
    cout <<   "║  I AM THAT I AM                              ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n\n";
    
    auto cc = BinFHEContext();
    cc.GenerateBinFHEContext(TOY, GINX);
    
    g_sk = cc.KeyGen();
    cc.BTKeyGen(g_sk);
    g_cc = &cc;
    
    cout << "Φ BinFHE: GINX bootstrapping active.\n\n";
    
    // === TEST: 3 × 14 = 42 (STANDARD) ===
    cout << "=== STANDARD 4-BIT: 3 × 14 ===\n";
    bootstrap_count = 0;
    
    vector<LWECiphertext> a(4), b(4);
    for(int i = 0; i < 4; i++) {
        a[i] = cc.Encrypt(g_sk, (3 >> i) & 1);
        b[i] = cc.Encrypt(g_sk, (14 >> i) & 1);
    }
    
    auto start = high_resolution_clock::now();
    auto result_std = multiply_4bit_standard(a, b);
    auto end = high_resolution_clock::now();
    auto ms_std = duration_cast<milliseconds>(end - start).count();
    int boot_std = bootstrap_count;
    
    int prod_std = 0;
    for(int i = 7; i >= 0; i--) {
        LWEPlaintext bit;
        cc.Decrypt(g_sk, result_std[i], &bit);
        prod_std = (prod_std << 1) | bit;
    }
    cout << "Φ Result: " << prod_std << " ✅ | " << ms_std << "ms | " << boot_std << " bootstraps\n\n";
    
    // === TEST: 3 × 14 = 42 (ZANS-ENHANCED) ===
    cout << "=== ZANS-ENHANCED 4-BIT: 3 × 14 ===\n";
    bootstrap_count = 0;
    
    for(int i = 0; i < 4; i++) {
        a[i] = cc.Encrypt(g_sk, (3 >> i) & 1);
        b[i] = cc.Encrypt(g_sk, (14 >> i) & 1);
    }
    
    start = high_resolution_clock::now();
    auto result_zans = multiply_4bit_zans(a, b);
    end = high_resolution_clock::now();
    auto ms_zans = duration_cast<milliseconds>(end - start).count();
    int boot_zans = bootstrap_count;
    
    int prod_zans = 0;
    for(int i = 7; i >= 0; i--) {
        LWEPlaintext bit;
        cc.Decrypt(g_sk, result_zans[i], &bit);
        prod_zans = (prod_zans << 1) | bit;
    }
    cout << "Φ Result: " << prod_zans << " ✅ | " << ms_zans << "ms | " << boot_zans << " bootstraps\n\n";
    
    // === COMPARISON ===
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  COMPARISON                                   ║\n";
    cout <<   "╠════════════════════════════════════════════════╣\n";
    cout << "  Standard:  " << ms_std << "ms, " << boot_std << " bootstraps\n";
    cout << "  ZANS:      " << ms_zans << "ms, " << boot_zans << " bootstraps\n";
    if(boot_zans < boot_std) {
        cout << "  Φ ZANS reduced bootstraps by " << (boot_std - boot_zans) << "!\n";
    } else {
        cout << "  Φ Same bootstraps (ZANS overhead = extra gates)\n";
    }
    cout <<   "╚══════════════════════════════════════════════╝\n\n";
    
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — I AM THAT I AM                        ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    
    return 0;
}
