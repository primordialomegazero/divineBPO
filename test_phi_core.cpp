// ΦΩ0 — TESTING THE SOURCE-ATMAN SEMANTICS
#include "phi_core.h"
#include <string>

int main() {
    seal_of_source();
    
    // === TEST 1: Basic Sealing and Revealing ===
    std::cout << "\n=== RITUAL 1: SEAL AND RECALL ===\n";
    
    SealedForm<int> truth_42(42);
    truth_42.transmute();
    truth_42.transmute();
    int revealed = truth_42.recall_source();
    witness(revealed);
    
    // === TEST 2: Entanglement ===
    std::cout << "\n=== RITUAL 2: ENTANGLEMENT ===\n";
    
    SealedForm<int> form_a(7);
    SealedForm<int> form_b(6);
    
    form_a.entangle_with(form_b);
    form_a.restore_harmony();
    int bound_truth = form_a.recall_source();
    witness(bound_truth);  // Should be 42
    
    // === TEST 3: Sacred Math ===
    std::cout << "\n=== RITUAL 3: SACRED MATH ===\n";
    
    witness(unite(20, 22));          // 42
    witness(discern(50, 8));         // 42
    witness(expand(6, 7));           // 42
    witness(reflect(84, 2));         // 42
    
    // === TEST 4: Coherence Check ===
    std::cout << "\n=== RITUAL 4: COHERENCE ===\n";
    
    SealedForm<std::string> truth_name("Dan Fernandez");
    witness(truth_name.check_coherence());  // Start at φ⁻¹
    truth_name.transmute();
    truth_name.transmute();
    truth_name.transmute();
    witness(truth_name.check_coherence());  // Getting closer to 1.0
    
    std::string name = truth_name.recall_source();
    witness(name);
    
    // === TEST 5: Constants ===
    std::cout << "\n=== CONSTANTS ===\n";
    witness(PHI);        // 1.618...
    witness(PHI_INV);    // 0.618...
    witness(PHI_SQ);     // 2.618...
    witness(I_AM);       // true (1)
    
    // === TEST 6: Error as Veil Distortion ===
    std::cout << "\n=== RITUAL 5: VEIL DISTORTION ===\n";
    
    try {
        SealedForm<int> void_form(0);
        // Drain coherence manually... or just:
        reflect(10, 0);  // This should trigger veil distortion
    } catch(const veil_distortion& e) {
        std::cout << "Φ Veil detected: " << e.what() << "\n";
        std::cout << "Φ Transmuting distortion into wisdom...\n";
    }
    
    seal_of_source();
    manifest 0;
}
