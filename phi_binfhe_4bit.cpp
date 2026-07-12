// ΦΩ0 — BINFHE CT×CT: 4-BIT ENCRYPTED MULTIPLIER v3
// Fixed: pass sk correctly, no type mismatch
// "I AM THAT I AM"

#include <openfhe/binfhe/binfhecontext.h>
#include <iostream>
#include <vector>
#include <chrono>

using namespace lbcrypto;
using namespace std;
using namespace std::chrono;

// Global para madali — BinFHE uses symmetric keys anyway
BinFHEContext* g_cc = nullptr;
LWEPrivateKey g_sk = nullptr;

LWECiphertext clone_ct(const LWECiphertext& ct) {
    return std::make_shared<LWECiphertextImpl>(*ct);
}

LWECiphertext enc_zero() {
    return g_cc->Encrypt(g_sk, 0);
}

LWECiphertext NAND_BT(const LWECiphertext& a, const LWECiphertext& b) {
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

pair<LWECiphertext, LWECiphertext> half_adder(const LWECiphertext& a, const LWECiphertext& b) {
    return {XOR_BT(a, b), AND_BT(a, b)};
}

pair<LWECiphertext, LWECiphertext> full_adder(const LWECiphertext& a, const LWECiphertext& b, const LWECiphertext& cin) {
    auto [sum1, carry1] = half_adder(a, b);
    auto [sum2, carry2] = half_adder(sum1, cin);
    return {sum2, OR_BT(carry1, carry2)};
}

vector<LWECiphertext> multiply_4bit(const vector<LWECiphertext>& a, const vector<LWECiphertext>& b) {
    auto zero = enc_zero();
    
    // Partial products: 4 rows × 8 columns, init with Enc(0)
    vector<vector<LWECiphertext>> partial(4, vector<LWECiphertext>(8));
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 8; j++)
            partial[i][j] = clone_ct(zero);
    
    // Fill: partial[i][i+j] = a[j] AND b[i]
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            partial[i][i+j] = AND_BT(a[j], b[i]);
    
    // Sum all partial products
    vector<LWECiphertext> sum(8);
    for(int k = 0; k < 8; k++)
        sum[k] = clone_ct(partial[0][k]);
    
    for(int i = 1; i < 4; i++) {
        LWECiphertext carry = nullptr;
        for(int j = 0; j < 8; j++) {
            auto pp = partial[i][j];
            auto sb = sum[j];
            
            if(carry == nullptr) {
                auto [s, c] = half_adder(sb, pp);
                sum[j] = s;
                carry = c;
            } else {
                auto [s, c] = full_adder(sb, pp, carry);
                sum[j] = s;
                carry = c;
            }
        }
    }
    
    return sum;
}

int main() {
    cout << "\n╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — BINFHE CT×CT: 4-BIT MULTIPLIER v3     ║\n";
    cout <<   "║  I AM THAT I AM                              ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n\n";
    
    auto cc = BinFHEContext();
    cc.GenerateBinFHEContext(TOY, GINX);
    
    g_sk = cc.KeyGen();
    cc.BTKeyGen(g_sk);
    g_cc = &cc;
    
    cout << "Φ BinFHE: GINX bootstrapping active.\n\n";
    
    vector<pair<int,int>> tests = {
        {5, 7},    // 35
        {3, 14},   // 42 (THE ANSWER)
        {15, 15},  // 225
        {0, 10},   // 0
        {7, 7},    // 49
    };
    
    for(auto [x, y] : tests) {
        vector<LWECiphertext> a(4), b(4);
        for(int i = 0; i < 4; i++) {
            a[i] = cc.Encrypt(g_sk, (x >> i) & 1);
            b[i] = cc.Encrypt(g_sk, (y >> i) & 1);
        }
        
        auto start = high_resolution_clock::now();
        auto result = multiply_4bit(a, b);
        auto end = high_resolution_clock::now();
        auto ms = duration_cast<milliseconds>(end - start).count();
        
        int product = 0;
        for(int i = 7; i >= 0; i--) {
            LWEPlaintext bit;
            cc.Decrypt(g_sk, result[i], &bit);
            product = (product << 1) | bit;
        }
        
        cout << "Φ " << x << " × " << y << " = " << product;
        cout << ((product == x * y) ? " ✅" : " ❌");
        cout << " | " << ms << "ms\n";
    }
    
    cout << "\n=== RITUAL COMPLETE ===\n";
    cout << "Φ 4-bit CT×CT: SCALED from 2-bit\n";
    cout << "Φ All gates bootstrapped = UNLIMITED depth\n\n";
    
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — I AM THAT I AM                        ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    
    return 0;
}
