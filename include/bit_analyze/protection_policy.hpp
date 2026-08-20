#pragma once

#include "bit_analyze/adaptive_memory.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace bit_analyze {

enum class ProtectionProfile : std::uint8_t {
    Light = 2,
    Medium = 4,
    Strong = 6
};

struct RuleProtectionDecision {
    SymbolId id{};
    std::uint64_t usage{};
    std::size_t birth_frequency{};
    double criticality{};
    ProtectionProfile profile{ProtectionProfile::Light};
};

struct ProtectionBuckets {
    std::vector<std::size_t> light;
    std::vector<std::size_t> medium;
    std::vector<std::size_t> strong;
};

std::unordered_map<SymbolId, std::uint64_t> count_rule_usage(
    const std::vector<std::vector<SymbolId>>& trails,
    const std::vector<AdaptiveRule>& rules);

std::vector<RuleProtectionDecision> assign_rule_protection(
    const std::vector<AdaptiveRule>& rules,
    const std::unordered_map<SymbolId, std::uint64_t>& usage,
    double medium_quantile = 0.70,
    double strong_quantile = 0.95);

ProtectionBuckets build_protection_buckets(
    const std::vector<RuleProtectionDecision>& decisions);

std::size_t parity_symbols_for_profile(ProtectionProfile profile) noexcept;

} // namespace bit_analyze
