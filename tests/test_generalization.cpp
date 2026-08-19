#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/hierarchical_memory.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace bit_analyze;

    const std::vector<std::vector<std::uint8_t>> train{
        {'A','B','A','B','A','B','A','B'},
        {'A','B','A','B','C','D','A','B'},
        {'A','B','C','D','A','B','C','D'}
    };

    const std::vector<std::uint8_t> unseen_shifted{
        'X','A','B','A','B','C','D','Y'
    };

    AdaptiveMemory adaptive;
    adaptive.train(train, 32, 2);
    const auto adaptive_rules_before = adaptive.rule_count();
    const auto adaptive_encoded = adaptive.encode(unseen_shifted);
    const auto adaptive_rules_after = adaptive.rule_count();
    assert(adaptive.decode(adaptive_encoded.trail) == unseen_shifted);
    assert(adaptive_rules_before == adaptive_rules_after);

    HierarchicalMemory fixed;
    for (const auto& sample : train) {
        const auto encoded = fixed.encode(sample, 3);
        assert(fixed.decode(encoded.trail) == sample);
    }

    const auto fixed_relations_before = fixed.relation_count();
    const auto fixed_encoded = fixed.encode(unseen_shifted, 3);
    const auto fixed_relations_after = fixed.relation_count();
    assert(fixed.decode(fixed_encoded.trail) == unseen_shifted);

    const auto new_fixed_relations = fixed_relations_after - fixed_relations_before;

    std::cout << "PASS: unseen-data generalization\n";
    std::cout << "Adaptive learned rules: " << adaptive_rules_before << "\n";
    std::cout << "Adaptive new rules on probe: "
              << (adaptive_rules_after - adaptive_rules_before) << "\n";
    std::cout << "Adaptive probe trail: " << adaptive_encoded.trail.size() << "\n";
    std::cout << "Fixed relations before probe: " << fixed_relations_before << "\n";
    std::cout << "Fixed new relations on probe: " << new_fixed_relations << "\n";
    std::cout << "Fixed probe trail: " << fixed_encoded.trail.size() << "\n";

    // The adaptive dictionary is frozen at inference. The fixed encoder is
    // allowed to mutate, so this test exposes the cost hidden by trail length.
    assert(new_fixed_relations > 0);

    return 0;
}
