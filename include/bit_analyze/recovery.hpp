#pragma once

#include "bit_analyze/adaptive_memory.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
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

std::vector<RuleParityGroup> build_rule_parity(
    const std::vector<AdaptiveRule>& rules,
    std::size_t group_size = 8);

std::optional<AdaptiveRule> recover_single_rule_from_parity(
    const RuleParityGroup& parity,
    const std::vector<AdaptiveRule>& rules,
    std::size_t missing_index);

} // namespace bit_analyze
