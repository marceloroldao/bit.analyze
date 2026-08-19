#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/hierarchical_memory.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace bit_analyze;

    const std::vector<std::vector<std::uint8_t>> corpus{
        {'A','B','A','B','A','B','A','B'},
        {'A','B','A','B','C','D','A','B'},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02}
    };

    AdaptiveMemory adaptive;
    adaptive.train(corpus, 16, 2);
    assert(adaptive.rule_count() > 0);

    const std::vector<std::uint8_t> probe{'A','B','A','B','A','B','C','D'};
    const auto adaptive_encoded = adaptive.encode(probe);
    const auto adaptive_decoded = adaptive.decode(adaptive_encoded.trail);
    assert(adaptive_decoded == probe);
    assert(adaptive_encoded.trail.size() < probe.size());

    HierarchicalMemory fixed;
    const auto fixed_encoded = fixed.encode(probe, 3);
    const auto fixed_decoded = fixed.decode(fixed_encoded.trail);
    assert(fixed_decoded == probe);

    const std::vector<std::uint8_t> shifted{'X','A','B','A','B','A','B','Y'};
    const auto adaptive_shifted = adaptive.encode(shifted);
    const auto fixed_shifted = fixed.encode(shifted, 3);
    assert(adaptive.decode(adaptive_shifted.trail) == shifted);
    assert(fixed.decode(fixed_shifted.trail) == shifted);

    std::cout << "PASS: adaptive lossless encode/decode\n";
    std::cout << "Learned rules: " << adaptive.rule_count() << "\n";
    std::cout << "Probe bytes: " << probe.size() << "\n";
    std::cout << "Adaptive trail: " << adaptive_encoded.trail.size() << "\n";
    std::cout << "Fixed trail: " << fixed_encoded.trail.size() << "\n";
    std::cout << "Shifted adaptive trail: " << adaptive_shifted.trail.size() << "\n";
    std::cout << "Shifted fixed trail: " << fixed_shifted.trail.size() << "\n";

    return 0;
}
