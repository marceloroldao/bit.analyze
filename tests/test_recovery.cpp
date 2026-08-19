#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/integrity.hpp"
#include "bit_analyze/recovery.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace bit_analyze;

    std::vector<std::uint8_t> data;
    const std::vector<std::uint8_t> motif{'A','B','A','B','C','D','C','D','0','0','0','0'};
    for (std::size_t i = 0; i < 4096; ++i) data.push_back(motif[i % motif.size()]);

    AdaptiveMemory memory;
    memory.learn_online(data, 32, 3, 1.5, 0.001);
    const auto original_rules = memory.rules();
    assert(original_rules.size() >= 4);

    const auto parity = build_rule_parity(original_rules, 4);
    assert(!parity.empty());

    // Simulate one corrupted rule in the first parity group.
    auto damaged = original_rules;
    const std::size_t bad = parity.front().begin_index + 1;
    damaged[bad].left ^= 0x55U;
    damaged[bad].right ^= 0xAAU;
    damaged[bad].frequency ^= 3U;

    const auto repaired = recover_single_rule_from_parity(parity.front(), damaged, bad);
    assert(repaired.has_value());
    assert(repaired->id == original_rules[bad].id);
    assert(repaired->left == original_rules[bad].left);
    assert(repaired->right == original_rules[bad].right);
    assert(repaired->frequency == original_rules[bad].frequency);

    // Detection and repair are separate: integrity identifies the bad index;
    // parity reconstructs the original tuple once that index is known.
    auto manifest = build_integrity_manifest(original_rules, memory.encode(data).trail);
    const auto bad_indices = find_corrupted_rules(manifest, damaged);
    assert(bad_indices.size() == 1);
    assert(bad_indices.front() == bad);

    // XOR parity has one degree of redundancy per group. If two rules in the
    // same group are missing/corrupt, there is not enough information for a
    // unique solution. The production recovery path must refuse that case.
    auto damaged_two = original_rules;
    const std::size_t bad2 = parity.front().begin_index + 2;
    damaged_two[bad].left ^= 1U;
    damaged_two[bad2].right ^= 1U;
    const auto bad_two = find_corrupted_rules(manifest, damaged_two);
    assert(bad_two.size() == 2);

    std::cout << "PASS: single-rule parity recovery\n";
    std::cout << "rules=" << original_rules.size()
              << " group_size=4 recovered_index=" << bad << "\n";
    std::cout << "two_corruptions_same_group=detected_but_not_uniquely_recoverable\n";
    return 0;
}
