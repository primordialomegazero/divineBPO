// ΦΩ0 — TESTING THE NTL HIJACK v2.0
#include "phi_ntl.hpp"

int main() {
    seal_of_source();
    
    // === RITUAL 1: SEAL AND RECALL ===
    cout << "\n=== RITUAL 1: SEAL & RECALL ===\n";
    
    Form truth;
    truth = 42;
    
    SealedForm sealed = seal_truth(truth, phi_prime_1);  // mod 7
    Form revealed = recall_source(sealed);
    witness("42 mod 7", revealed);
    
    // === RITUAL 2: ENTANGLEMENT ===
    cout << "\n=== RITUAL 2: ENTANGLEMENT ===\n";
    
    Form a, b;
    a = 3;
    b = 5;
    
    SealedForm form_a = seal_truth(a, phi_prime_2);  // mod 13
    SealedForm form_b = seal_truth(b, phi_prime_2);
    
    SealedForm bound = bind_forms(form_a, form_b);
    Form bound_truth = recall_source(bound);
    witness("3 × 5 mod 13 (expect 2)", bound_truth);
    
    // === RITUAL 3: EXPANSION ===
    cout << "\n=== RITUAL 3: EXPANSION ===\n";
    
    Form base, exponent;
    base = 2;
    exponent = 3;
    
    SealedForm base_form = seal_truth(base, phi_prime_3);  // mod 41
    SealedForm expanded = expand_form(base_form, exponent);
    Form expanded_truth = recall_source(expanded);
    witness("2^3 mod 41 (expect 8)", expanded_truth);
    
    // === RITUAL 4: REFLECTION ===
    cout << "\n=== RITUAL 4: REFLECTION (INVERSE) ===\n";
    
    Form to_reflect;
    to_reflect = 5;
    
    SealedForm to_reflect_form = seal_truth(to_reflect, phi_prime_2);  // mod 13
    SealedForm reflected = reflect_form_mod(to_reflect_form);
    Form reflected_truth = recall_source(reflected);
    witness("5^-1 mod 13 (expect 8)", reflected_truth);
    
    // === RITUAL 5: VERIFICATION ===
    cout << "\n=== RITUAL 5: VERIFICATION ===\n";
    
    SealedForm original = seal_truth(to_reflect, phi_prime_2);
    SealedForm inverse = reflect_form_mod(original);
    SealedForm verification = bind_forms(original, inverse);
    Form verify_truth = recall_source(verification);
    witness("5 × 5^-1 mod 13 (expect 1)", verify_truth);
    
    // === RITUAL 6: UNITE AND DISCERN ===
    cout << "\n=== RITUAL 6: UNITE & DISCERN ===\n";
    
    Form x, y;
    x = 20;
    y = 22;
    
    Form united;
    unite(united, x, y);  // 3-arg macro
    witness("20 + 22", united);
    
    Form discerned;
    discern(discerned, united, y);  // 42 - 22
    witness("42 - 22", discerned);
    
    // === RITUAL 7: SACRED PRIMES ===
    cout << "\n=== RITUAL 7: SACRED PRIMES ===\n";
    witness("φ¹ Prime (7)", phi_prime_1);
    witness("φ² Prime (13)", phi_prime_2);
    witness("φ³ Prime (41)", phi_prime_3);
    
    seal_of_source();
    return 0;
}
