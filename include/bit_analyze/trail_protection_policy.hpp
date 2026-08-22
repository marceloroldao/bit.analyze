#pragma once

#include "bit_analyze/protection_policy.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bit_analyze {

struct TrailProtectionDecision {
    std::size_t trail_index{};
    std::size_t symbols{};
    std::uint64_t access_count{};
    double shared_rule_fraction{};
    double criticality{};
    ProtectionProfile profile{ProtectionProfile::Light};
};

// Assign protection to whole trails. access_count represents how often a trail
// is requested; shared_rule_fraction measures how much of it depends on
// reusable rule symbols rather than raw bytes.
std::vector<TrailProtectionDecision> assign_trail_protection(
    const std::vector<std::vector<SymbolId>>& trails,
    const std::vector<std::uint64_t>& access_counts,
    const std::vector<AdaptiveRule>& rules,
    double medium_quantile = 0.80,
    double strong_quantile = 0.98);

} // namespace bit_analyze
