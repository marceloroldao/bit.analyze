#pragma once

#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/protection_policy.hpp"
#include "bit_analyze/recovery.hpp"
#include "bit_analyze/trail_recovery.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bit_analyze {

struct ProtectedTrailState {
    std::vector<SymbolId> trail;
    ProtectionProfile profile{ProtectionProfile::Light};
    std::uint64_t trail_hash{};
    std::size_t block_size{64};
    std::vector<std::uint64_t> block_hashes;
    std::vector<TrailDualParityBlock> dual_parity;
};

struct ProtectedMemorySnapshot {
    std::uint32_t version{2};
    std::size_t interleave_lanes{64};
    std::vector<AdaptiveRule> rules;
    std::vector<ProtectionProfile> rule_profiles;
    std::vector<std::uint64_t> rule_hashes;
    std::vector<RuleDualParityGroup> rule_dual_parity;
    std::vector<ProtectedTrailState> trails;
};

void save_protected_snapshot(const ProtectedMemorySnapshot& snapshot,
                             const std::string& path);
ProtectedMemorySnapshot load_protected_snapshot(const std::string& path);

} // namespace bit_analyze
