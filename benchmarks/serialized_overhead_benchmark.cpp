#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/integrity.hpp"
#include "bit_analyze/persistence.hpp"
#include "bit_analyze/protected_persistence.hpp"
#include "bit_analyze/recovery.hpp"
#include "bit_analyze/trail_recovery.hpp"

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

std::uint64_t size_of(const char* path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    return in ? static_cast<std::uint64_t>(in.tellg()) : 0ULL;
}

} // namespace

int main() {
    using namespace bit_analyze;

    std::vector<std::uint8_t> data;
    const std::vector<std::uint8_t> motif{'A','B','A','B','C','D','C','D','0','0','0','0'};
    constexpr std::size_t kBytes = 256 * 1024;
    data.reserve(kBytes);
    for (std::size_t i = 0; i < kBytes; ++i) data.push_back(motif[i % motif.size()]);

    AdaptiveMemory memory;
    memory.learn_online(data, 64, 3, 1.2, 0.001);
    const auto encoded = memory.encode(data);

    MemorySnapshot basic;
    basic.rules = memory.rules();
    basic.trails = {encoded.trail};

    ProtectedMemorySnapshot protected_snapshot;
    protected_snapshot.rules = memory.rules();
    protected_snapshot.rule_profiles.assign(memory.rules().size(), ProtectionProfile::Light);
    for (const auto& r : memory.rules()) protected_snapshot.rule_hashes.push_back(hash_rule(r));
    protected_snapshot.rule_dual_parity = build_rule_dual_parity(memory.rules(), 8);

    const auto manifest = build_integrity_manifest(memory.rules(), encoded.trail, 64);
    ProtectedTrailState t;
    t.trail = encoded.trail;
    t.profile = ProtectionProfile::Light;
    t.trail_hash = manifest.trail_hash;
    t.block_size = manifest.trail_block_size;
    t.block_hashes = manifest.trail_block_hashes;
    t.dual_parity = build_trail_dual_parity(encoded.trail, 64);
    protected_snapshot.trails.push_back(std::move(t));

    const char* basic_path = "bit_analyze_basic_overhead.bin";
    const char* protected_path = "bit_analyze_protected_overhead.bin";
    save_snapshot(basic, basic_path);
    save_protected_snapshot(protected_snapshot, protected_path);

    const auto basic_bytes = size_of(basic_path);
    const auto protected_bytes = size_of(protected_path);
    const double overhead = basic_bytes == 0 ? 0.0 :
        100.0 * (static_cast<double>(protected_bytes) / static_cast<double>(basic_bytes) - 1.0);

    std::cout << "input_bytes=" << data.size() << '\n';
    std::cout << "rules=" << memory.rule_count() << '\n';
    std::cout << "trail_symbols=" << encoded.trail.size() << '\n';
    std::cout << "basic_snapshot_bytes=" << basic_bytes << '\n';
    std::cout << "protected_snapshot_bytes=" << protected_bytes << '\n';
    std::cout << std::fixed << std::setprecision(3)
              << "serialized_protection_overhead_pct=" << overhead << '\n';

    std::remove(basic_path);
    std::remove(protected_path);
    return 0;
}
