#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/integrity.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace bit_analyze;

    std::vector<std::uint8_t> data;
    const std::vector<std::uint8_t> motif{
        'A','B','A','B','A','B','A','B','0','0','0','0','X','Y','X','Y'
    };
    for (std::size_t i = 0; i < 4096; ++i) data.push_back(motif[i % motif.size()]);

    AdaptiveMemory memory;
    memory.learn_online(data, 16, 2, 1.0, 0.0);
    const auto encoded = memory.encode(data);
    assert(memory.decode(encoded.trail) == data);

    constexpr std::size_t kBlockSize = 8;
    const auto manifest = build_integrity_manifest(memory.rules(), encoded.trail, kBlockSize);

    assert(find_corrupted_rules(manifest, memory.rules()).empty());
    assert(verify_trail(manifest, encoded.trail));
    assert(find_corrupted_trail_blocks(manifest, encoded.trail).empty());

    auto corrupted_rules = memory.rules();
    assert(!corrupted_rules.empty());
    const std::size_t target = corrupted_rules.size() / 2;
    corrupted_rules[target].right ^= 1ULL;

    const auto bad = find_corrupted_rules(manifest, corrupted_rules);
    assert(bad.size() == 1);
    assert(bad.front() == target);

    auto corrupted_trail = encoded.trail;
    assert(!corrupted_trail.empty());
    const std::size_t trail_index = corrupted_trail.size() / 2;
    corrupted_trail[trail_index] ^= 1ULL;
    assert(!verify_trail(manifest, corrupted_trail));

    const auto bad_blocks = find_corrupted_trail_blocks(manifest, corrupted_trail);
    assert(bad_blocks.size() == 1);
    assert(bad_blocks.front() == trail_index / kBlockSize);

    std::cout << "PASS: corruption detection and block localization\n";
    std::cout << "rules=" << memory.rule_count() << '\n';
    std::cout << "corrupted_rule_index=" << target << '\n';
    std::cout << "trail_symbols=" << encoded.trail.size()
              << " corrupted_trail_block=" << bad_blocks.front() << '\n';

    return 0;
}
