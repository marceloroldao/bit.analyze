#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/integrity.hpp"
#include "bit_analyze/recovery.hpp"
#include "bit_analyze/trail_recovery.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace bit_analyze;

    std::vector<std::uint8_t> data;
    const std::vector<std::uint8_t> motif{
        'A','B','A','B','C','D','C','D','0','0','0','0','X','Y','X','Y'
    };
    for (std::size_t i = 0; i < 32768; ++i) {
        data.push_back(motif[i % motif.size()]);
    }

    AdaptiveMemory memory;
    memory.learn_online(data, 48, 3, 1.5, 0.001);
    const auto encoded = memory.encode(data);
    assert(memory.decode(encoded.trail) == data);
    assert(encoded.trail.size() >= 8);

    constexpr std::size_t kBlock = 8;
    const auto manifest = build_integrity_manifest(memory.rules(), encoded.trail, kBlock);
    const auto single_parity = build_trail_parity(encoded.trail, kBlock);
    const auto dual_parity = build_trail_dual_parity(encoded.trail, kBlock);
    assert(!single_parity.empty());
    assert(!dual_parity.empty());

    // One-symbol corruption. Only the block is localized; the recovery code
    // infers the exact position by parity + the stored block hash.
    auto damaged_one = encoded.trail;
    const std::size_t block_one = single_parity.size() > 1 ? 1 : 0;
    const auto begin_one = single_parity[block_one].begin_index;
    const auto pos_one = begin_one + (single_parity[block_one].count / 2);
    damaged_one[pos_one] ^= 0x1234ULL;

    const auto bad_blocks_one = find_corrupted_trail_blocks(manifest, damaged_one);
    assert(bad_blocks_one.size() == 1);
    assert(bad_blocks_one.front() == block_one);

    const auto repaired_one = recover_one_symbol_in_block(
        manifest, single_parity[block_one], damaged_one, block_one);
    assert(repaired_one.has_value());
    assert(*repaired_one == encoded.trail);
    assert(memory.decode(*repaired_one) == data);

    // Two-symbol corruption in the same block. Exact positions are again not
    // supplied to the recovery function.
    auto damaged_two = encoded.trail;
    const std::size_t block_two = dual_parity.size() > 2 ? 2 : 0;
    const auto begin_two = dual_parity[block_two].begin_index;
    assert(dual_parity[block_two].count >= 2);
    const auto pos_a = begin_two;
    const auto pos_b = begin_two + dual_parity[block_two].count - 1;
    damaged_two[pos_a] ^= 0x55ULL;
    damaged_two[pos_b] ^= 0xAA00ULL;

    const auto bad_blocks_two = find_corrupted_trail_blocks(manifest, damaged_two);
    assert(bad_blocks_two.size() == 1);
    assert(bad_blocks_two.front() == block_two);

    const auto repaired_two = recover_two_symbols_in_block(
        manifest, dual_parity[block_two], damaged_two, block_two);
    assert(repaired_two.has_value());
    assert(*repaired_two == encoded.trail);
    assert(memory.decode(*repaired_two) == data);

    // Combined corruption: one dictionary rule plus two trail symbols.
    const auto original_rules = memory.rules();
    assert(!original_rules.empty());
    const auto rule_parity = build_rule_parity(original_rules, 8);
    assert(!rule_parity.empty());

    auto damaged_rules = original_rules;
    const std::size_t bad_rule = rule_parity.front().begin_index;
    damaged_rules[bad_rule].left ^= 0x77ULL;
    damaged_rules[bad_rule].frequency ^= 1U;

    const auto bad_rule_indices = find_corrupted_rules(manifest, damaged_rules);
    assert(bad_rule_indices.size() == 1);
    assert(bad_rule_indices.front() == bad_rule);

    const auto repaired_rule = recover_single_rule_from_parity(
        rule_parity.front(), damaged_rules, bad_rule);
    assert(repaired_rule.has_value());
    assert(repaired_rule->id == original_rules[bad_rule].id);
    assert(repaired_rule->left == original_rules[bad_rule].left);
    assert(repaired_rule->right == original_rules[bad_rule].right);
    assert(repaired_rule->frequency == original_rules[bad_rule].frequency);

    auto combined_trail = encoded.trail;
    combined_trail[pos_a] ^= 0x101ULL;
    combined_trail[pos_b] ^= 0x202ULL;
    const auto repaired_combined_trail = recover_two_symbols_in_block(
        manifest, dual_parity[block_two], combined_trail, block_two);
    assert(repaired_combined_trail.has_value());
    assert(*repaired_combined_trail == encoded.trail);
    assert(memory.decode(*repaired_combined_trail) == data);

    std::cout << "PASS: trail parity recovery and combined corruption recovery\n";
    std::cout << "trail_symbols=" << encoded.trail.size()
              << " block_size=" << kBlock
              << " rules=" << original_rules.size() << '\n';
    std::cout << "one_symbol_recovered=1 two_symbols_recovered=1 combined_recovered=1\n";
    return 0;
}
