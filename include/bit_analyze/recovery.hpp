#pragma once

#include "bit_analyze/adaptive_memory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace bit_analyze {

struct RuleParityGroup {
    std::size_t begin_index{};
    std::size_t count{};
    std::uint64_t xor_id{};
    std::uint64_t xor_left{};
    std::uint64_t xor_right{};
    std::uint64_t xor_frequency{};
};

struct RuleDualParityGroup {
    std::size_t begin_index{};
    std::size_t count{};
    std::array<std::uint8_t, 8> p_id{};
    std::array<std::uint8_t, 8> q_id{};
    std::array<std::uint8_t, 8> p_left{};
    std::array<std::uint8_t, 8> q_left{};
    std::array<std::uint8_t, 8> p_right{};
    std::array<std::uint8_t, 8> q_right{};
    std::array<std::uint8_t, 8> p_frequency{};
    std::array<std::uint8_t, 8> q_frequency{};
};

std::vector<RuleParityGroup> build_rule_parity(
    const std::vector<AdaptiveRule>& rules,
    std::size_t group_size = 8);

std::optional<AdaptiveRule> recover_single_rule_from_parity(
    const RuleParityGroup& parity,
    const std::vector<AdaptiveRule>& rules,
    std::size_t missing_index);

std::vector<RuleDualParityGroup> build_rule_dual_parity(
    const std::vector<AdaptiveRule>& rules,
    std::size_t group_size = 8);

std::optional<std::pair<AdaptiveRule, AdaptiveRule>> recover_two_rules_from_dual_parity(
    const RuleDualParityGroup& parity,
    const std::vector<AdaptiveRule>& rules,
    std::size_t missing_index_a,
    std::size_t missing_index_b);

} // namespace bit_analyze
