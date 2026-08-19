#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/integrity.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace bit_analyze;

    const std::vector<std::uint8_t> data{
        'A','B','A','B','A','B','A','B',
        '0','0','0','0','X','Y','X','Y'
    };

    AdaptiveMemory memory;
    memory.learn_online(data, 16, 2, 1.0, 0.0);
    const auto encoded = memory.encode(data);
    assert(memory.decode(encoded.trail) == data);

    const auto manifest = build_integrity_manifest(memory.rules(), encoded.trail);

    assert(find_corrupted_rules(manifest, memory.rules()).empty());
    assert(verify_trail(manifest, encoded.trail));

    auto corrupted_rules = memory.rules();
    assert(!corrupted_rules.empty());
    const std::size_t target = corrupted_rules.size() / 2;
    corrupted_rules[target].right ^= 1ULL;

    const auto bad = find_corrupted_rules(manifest, corrupted_rules);
    assert(bad.size() == 1);
    assert(bad.front() == target);

    auto corrupted_trail = encoded.trail;
    assert(!corrupted_trail.empty());
    corrupted_trail[corrupted_trail.size() / 2] ^= 1ULL;
    assert(!verify_trail(manifest, corrupted_trail));

    std::cout << "PASS: corruption detection and localization\n";
    std::cout << "rules=" << memory.rule_count() << '\n';
    std::cout << "corrupted_rule_index=" << target << '\n';
    std::cout << "trail_symbols=" << encoded.trail.size() << '\n';

    return 0;
}
