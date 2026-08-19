#include "bit_analyze/recovery.hpp"

#include <algorithm>

namespace bit_analyze {

std::vector<RuleParityGroup> build_rule_parity(
    const std::vector<AdaptiveRule>& rules,
    std::size_t group_size) {
    std::vector<RuleParityGroup> groups;
    if (group_size == 0) return groups;

    for (std::size_t begin = 0; begin < rules.size(); begin += group_size) {
        RuleParityGroup g;
        g.begin_index = begin;
        g.count = std::min(group_size, rules.size() - begin);

        for (std::size_t i = begin; i < begin + g.count; ++i) {
            const auto& r = rules[i];
            g.xor_id ^= static_cast<std::uint64_t>(r.id);
            g.xor_left ^= static_cast<std::uint64_t>(r.left);
            g.xor_right ^= static_cast<std::uint64_t>(r.right);
            g.xor_frequency ^= static_cast<std::uint64_t>(r.frequency);
        }
        groups.push_back(g);
    }
    return groups;
}

std::optional<AdaptiveRule> recover_single_rule_from_parity(
    const RuleParityGroup& parity,
    const std::vector<AdaptiveRule>& rules,
    std::size_t missing_index) {
    if (missing_index < parity.begin_index ||
        missing_index >= parity.begin_index + parity.count ||
        parity.begin_index + parity.count > rules.size()) {
        return std::nullopt;
    }

    std::uint64_t id = parity.xor_id;
    std::uint64_t left = parity.xor_left;
    std::uint64_t right = parity.xor_right;
    std::uint64_t frequency = parity.xor_frequency;

    for (std::size_t i = parity.begin_index;
         i < parity.begin_index + parity.count; ++i) {
        if (i == missing_index) continue;
        const auto& r = rules[i];
        id ^= static_cast<std::uint64_t>(r.id);
        left ^= static_cast<std::uint64_t>(r.left);
        right ^= static_cast<std::uint64_t>(r.right);
        frequency ^= static_cast<std::uint64_t>(r.frequency);
    }

    return AdaptiveRule{
        static_cast<SymbolId>(id),
        static_cast<SymbolId>(left),
        static_cast<SymbolId>(right),
        static_cast<std::size_t>(frequency)
    };
}

} // namespace bit_analyze
