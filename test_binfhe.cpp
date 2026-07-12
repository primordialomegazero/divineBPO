// ΦΩ0 — BINFHE BOOTSTRAP TEST
#include <openfhe/binfhe/binfhecontext.h>
#include <iostream>

using namespace lbcrypto;
using namespace std;

int main() {
    cout << "\n╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — BINFHE BOOTSTRAP TEST                  ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n\n";
    
    // Generate binFHE context with bootstrapping
    auto cc = BinFHEContext();
    cc.GenerateBinFHEContext(TOY, GINX);
    
    cout << "Φ BinFHE context created (TOY security, GINX bootstrapping)\n\n";
    
    // Generate keys
    auto sk = cc.KeyGen();
    cc.BTKeyGen(sk);
    cout << "Φ Keys generated with bootstrapping.\n\n";
    
    // Encrypt a bit
    auto ct1 = cc.Encrypt(sk, 1);
    auto ct0 = cc.Encrypt(sk, 0);
    cout << "Φ Bits encrypted: 1 and 0\n\n";
    
    // NAND gate (with bootstrapping!)
    cout << "Φ Computing 1 NAND 0...\n";
    auto ct_nand = cc.EvalBinGate(NAND, ct1, ct0);
    
    // Decrypt
    LWEPlaintext result;
    cc.Decrypt(sk, ct_nand, &result);
    cout << "Φ Result: " << result << " (expected: 1)\n\n";
    
    // Bootstrap
    cout << "Φ Testing bootstrapping...\n";
    auto ct_boot = cc.Bootstrap(ct_nand);
    cc.Decrypt(sk, ct_boot, &result);
    cout << "Φ After bootstrap: " << result << "\n\n";
    
    cout << "╔══════════════════════════════════════════════╗\n";
    cout <<   "║  ΦΩ0 — I AM THAT I AM                        ║\n";
    cout <<   "╚══════════════════════════════════════════════╝\n";
    
    return 0;
}
