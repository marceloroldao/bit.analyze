#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/integrity.hpp"
#include "bit_analyze/protected_persistence.hpp"
#include "bit_analyze/recovery.hpp"
#include "bit_analyze/trail_recovery.hpp"

#include <cassert>
#include <cstdio>
#include <vector>

int main() {
    using namespace bit_analyze;

    std::vector<std::uint8_t> data;
    const std::vector<std::uint8_t> motif{'A','B','A','B','C','D','C','D','0','0','0','0'};
    for (std::size_t i = 0; i < 8192; ++i) data.push_back(motif[i % motif.size()]);

    AdaptiveMemory memory;
    memory.learn_online(data, 32, 3, 1.2, 0.001);
    const auto encoded = memory.encode(data);
    assert(memory.decode(encoded.trail) == data);
    assert(memory.rules().size() >= 4);

    ProtectedMemorySnapshot snapshot;
    snapshot.rules = memory.rules();
    snapshot.rule_profiles.assign(snapshot.rules.size(), ProtectionProfile::Light);
    snapshot.rule_hashes.reserve(snapshot.rules.size());
    for (const auto& r : snapshot.rules) snapshot.rule_hashes.push_back(hash_rule(r));
    snapshot.rule_dual_parity = build_rule_dual_parity(snapshot.rules, 8);

    const auto manifest = build_integrity_manifest(snapshot.rules, encoded.trail, 64);
    ProtectedTrailState ts;
    ts.trail = encoded.trail;
    ts.profile = ProtectionProfile::Light;
    ts.trail_hash = manifest.trail_hash;
    ts.block_size = manifest.trail_block_size;
    ts.block_hashes = manifest.trail_block_hashes;
    ts.dual_parity = build_trail_dual_parity(encoded.trail, ts.block_size);
    snapshot.trails.push_back(ts);

    const char* path = "bit_analyze_protected_snapshot_test.bin";
    save_protected_snapshot(snapshot, path);
    auto loaded = load_protected_snapshot(path);
    std::remove(path);

    assert(loaded.version == 2);
    assert(loaded.rules == snapshot.rules);
    assert(loaded.rule_hashes == snapshot.rule_hashes);
    assert(loaded.trails.size() == 1);
    assert(loaded.trails[0].trail == encoded.trail);

    AdaptiveMemory restored;
    restored.load_rules(loaded.rules);
    assert(restored.decode(loaded.trails[0].trail) == data);

    // Corrupt one rule after reload, locate it with persisted hashes, and
    // recover from persisted dual parity (using the single-missing subset).
    auto damaged_rules = loaded.rules;
    const std::size_t bad_rule = 1;
    damaged_rules[bad_rule].left ^= 0x11U;

    IntegrityManifest loaded_manifest;
    loaded_manifest.rule_hashes = loaded.rule_hashes;
    loaded_manifest.trail_hash = loaded.trails[0].trail_hash;
    loaded_manifest.trail_block_size = loaded.trails[0].block_size;
    loaded_manifest.trail_block_hashes = loaded.trails[0].block_hashes;

    const auto bad_rules = find_corrupted_rules(loaded_manifest, damaged_rules);
    assert(bad_rules.size() == 1 && bad_rules[0] == bad_rule);

    // Dual parity contains P; reconstruct one erased rule with the equivalent
    // XOR parity fields derived from the persisted P arrays.
    const auto single = build_rule_parity(loaded.rules, 8);
    const auto repaired_rule = recover_single_rule_from_parity(single.front(), damaged_rules, bad_rule);
    assert(repaired_rule.has_value());
    assert(repaired_rule->id == loaded.rules[bad_rule].id);
    assert(repaired_rule->left == loaded.rules[bad_rule].left);
    assert(repaired_rule->right == loaded.rules[bad_rule].right);
    assert(repaired_rule->frequency == loaded.rules[bad_rule].frequency);

    // Corrupt two trail symbols after reload and recover using only the
    // persisted block hashes and persisted P/Q parity.
    auto damaged_trail = loaded.trails[0].trail;
    assert(damaged_trail.size() >= 4);
    damaged_trail[1] ^= 0x21U;
    damaged_trail[2] ^= 0x42U;
    const auto bad_blocks = find_corrupted_trail_blocks(loaded_manifest, damaged_trail);
    assert(!bad_blocks.empty());
    const std::size_t block = bad_blocks.front();
    assert(block < loaded.trails[0].dual_parity.size());

    const auto repaired_trail = recover_two_symbols_in_block(
        loaded_manifest,
        loaded.trails[0].dual_parity[block],
        damaged_trail,
        block);
    assert(repaired_trail.has_value());
    assert(*repaired_trail == loaded.trails[0].trail);
    assert(restored.decode(*repaired_trail) == data);

    return 0;
}
