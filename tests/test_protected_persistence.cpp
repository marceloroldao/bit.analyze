#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/integrity.hpp"
#include "bit_analyze/protected_persistence.hpp"
#include "bit_analyze/recovery.hpp"
#include "bit_analyze/trail_recovery.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <vector>

namespace {

bool same_rule(const bit_analyze::AdaptiveRule& a,
               const bit_analyze::AdaptiveRule& b) {
    return a.id == b.id && a.left == b.left && a.right == b.right &&
           a.frequency == b.frequency;
}

} // namespace

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
    assert(!snapshot.rule_dual_parity.empty());

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

    assert(loaded.version == 2);
    assert(loaded.rules.size() == snapshot.rules.size());
    for (std::size_t i = 0; i < loaded.rules.size(); ++i)
        assert(same_rule(loaded.rules[i], snapshot.rules[i]));
    assert(loaded.rule_hashes == snapshot.rule_hashes);
    assert(loaded.rule_profiles == snapshot.rule_profiles);
    assert(loaded.rule_dual_parity.size() == snapshot.rule_dual_parity.size());
    assert(loaded.trails.size() == 1);
    assert(loaded.trails[0].trail == encoded.trail);

    AdaptiveMemory restored;
    restored.load_rules(loaded.rules);
    assert(restored.decode(loaded.trails[0].trail) == data);

    IntegrityManifest loaded_manifest;
    loaded_manifest.rule_hashes = loaded.rule_hashes;
    loaded_manifest.trail_hash = loaded.trails[0].trail_hash;
    loaded_manifest.trail_block_size = loaded.trails[0].block_size;
    loaded_manifest.trail_block_hashes = loaded.trails[0].block_hashes;

    auto damaged_rules = loaded.rules;
    const std::size_t bad_a = 1;
    const std::size_t bad_b = 2;
    damaged_rules[bad_a].left ^= 0x11U;
    damaged_rules[bad_b].right ^= 0x22U;

    const auto bad_rules = find_corrupted_rules(loaded_manifest, damaged_rules);
    assert(bad_rules.size() == 2);
    assert(bad_rules[0] == bad_a && bad_rules[1] == bad_b);

    const auto repaired_rules = recover_two_rules_from_dual_parity(
        loaded.rule_dual_parity.front(), damaged_rules, bad_a, bad_b);
    assert(repaired_rules.has_value());
    assert(same_rule(repaired_rules->first, loaded.rules[bad_a]));
    assert(same_rule(repaired_rules->second, loaded.rules[bad_b]));

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

    // Corrupt one byte in the serialized container itself. The file checksum
    // must reject the snapshot before any persisted state is trusted.
    {
        std::fstream f(path, std::ios::binary | std::ios::in | std::ios::out);
        assert(f.good());
        f.seekg(16);
        char byte = 0;
        f.read(&byte, 1);
        assert(f.good());
        byte ^= 0x01;
        f.seekp(16);
        f.write(&byte, 1);
        assert(f.good());
    }

    bool rejected = false;
    try {
        (void)load_protected_snapshot(path);
    } catch (...) {
        rejected = true;
    }
    assert(rejected);

    std::remove(path);
    return 0;
}
