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
    const auto dual = build_rule_dual_parity(original_rules, 4);
    assert(!parity.empty());
    assert(!dual.empty());

    // One corrupted rule: simple XOR parity is enough.
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

    const auto manifest = build_integrity_manifest(original_rules, memory.encode(data).trail);
    const auto bad_indices = find_corrupted_rules(manifest, damaged);
    assert(bad_indices.size() == 1);
    assert(bad_indices.front() == bad);

    // Two corrupted rules: dual P/Q parity over GF(256) recovers both exactly,
    // provided integrity has already identified their indices.
    auto damaged_two = original_rules;
    const std::size_t bad2 = dual.front().begin_index + 2;
    damaged_two[bad].id ^= 0x101U;
    damaged_two[bad].left ^= 0x123456U;
    damaged_two[bad].frequency ^= 7U;
    damaged_two[bad2].right ^= 0xABCDEU;
    damaged_two[bad2].frequency ^= 11U;

    const auto bad_two = find_corrupted_rules(manifest, damaged_two);
    assert(bad_two.size() == 2);

    const auto recovered_two = recover_two_rules_from_dual_parity(
        dual.front(), damaged_two, bad, bad2);
    assert(recovered_two.has_value());

    const auto& ra = recovered_two->first;
    const auto& rb = recovered_two->second;
    assert(ra.id == original_rules[bad].id);
    assert(ra.left == original_rules[bad].left);
    assert(ra.right == original_rules[bad].right);
    assert(ra.frequency == original_rules[bad].frequency);
    assert(rb.id == original_rules[bad2].id);
    assert(rb.left == original_rules[bad2].left);
    assert(rb.right == original_rules[bad2].right);
    assert(rb.frequency == original_rules[bad2].frequency);

    std::cout << "PASS: one-rule XOR and two-rule GF256 recovery\n";
    std::cout << "rules=" << original_rules.size()
              << " group_size=4 recovered_indices=" << bad << ',' << bad2 << "\n";
    std::cout << "three_corruptions_same_group=outside_dual_parity_capacity\n";
    return 0;
}
