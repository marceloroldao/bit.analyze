#include "bit_analyze/recovery.hpp"

#include <algorithm>

namespace bit_analyze {
namespace {

std::uint8_t gf_mul(std::uint8_t a, std::uint8_t b) {
    std::uint8_t p = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1U) p ^= a;
        const bool hi = (a & 0x80U) != 0;
        a <<= 1U;
        if (hi) a ^= 0x1dU;
        b >>= 1U;
    }
    return p;
}

std::uint8_t gf_pow(std::uint8_t a, int e) {
    std::uint8_t r = 1;
    while (e > 0) {
        if (e & 1) r = gf_mul(r, a);
        a = gf_mul(a, a);
        e >>= 1;
    }
    return r;
}

std::uint8_t gf_inv(std::uint8_t a) {
    return a == 0 ? 0 : gf_pow(a, 254);
}

std::array<std::uint8_t, 8> to_bytes(std::uint64_t v) {
    std::array<std::uint8_t, 8> out{};
    for (std::size_t i = 0; i < 8; ++i) out[i] = static_cast<std::uint8_t>((v >> (8U * i)) & 0xffU);
    return out;
}

std::uint64_t from_bytes(const std::array<std::uint8_t, 8>& b) {
    std::uint64_t v = 0;
    for (std::size_t i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(b[i]) << (8U * i);
    return v;
}

void parity_accumulate(std::array<std::uint8_t,8>& p,
                       std::array<std::uint8_t,8>& q,
                       std::uint64_t value,
                       std::uint8_t coeff) {
    const auto bytes = to_bytes(value);
    for (std::size_t i = 0; i < 8; ++i) {
        p[i] ^= bytes[i];
        q[i] ^= gf_mul(coeff, bytes[i]);
    }
}

std::pair<std::uint64_t,std::uint64_t> recover_pair_field(
    const std::array<std::uint8_t,8>& p,
    const std::array<std::uint8_t,8>& q,
    std::uint8_t ca,
    std::uint8_t cb,
    const std::vector<std::pair<std::uint64_t,std::uint8_t>>& known) {

    auto pp = p;
    auto qq = q;
    for (const auto& [value, coeff] : known) {
        const auto bytes = to_bytes(value);
        for (std::size_t i = 0; i < 8; ++i) {
            pp[i] ^= bytes[i];
            qq[i] ^= gf_mul(coeff, bytes[i]);
        }
    }

    std::array<std::uint8_t,8> a{}, b{};
    const std::uint8_t denom = ca ^ cb;
    const std::uint8_t inv = gf_inv(denom);
    for (std::size_t i = 0; i < 8; ++i) {
        const std::uint8_t rhs = qq[i] ^ gf_mul(cb, pp[i]);
        a[i] = gf_mul(rhs, inv);
        b[i] = pp[i] ^ a[i];
    }
    return {from_bytes(a), from_bytes(b)};
}

} // namespace

std::vector<RuleParityGroup> build_rule_parity(const std::vector<AdaptiveRule>& rules, std::size_t group_size) {
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

std::optional<AdaptiveRule> recover_single_rule_from_parity(const RuleParityGroup& parity, const std::vector<AdaptiveRule>& rules, std::size_t missing_index) {
    if (missing_index < parity.begin_index || missing_index >= parity.begin_index + parity.count || parity.begin_index + parity.count > rules.size()) return std::nullopt;
    std::uint64_t id = parity.xor_id, left = parity.xor_left, right = parity.xor_right, frequency = parity.xor_frequency;
    for (std::size_t i = parity.begin_index; i < parity.begin_index + parity.count; ++i) {
        if (i == missing_index) continue;
        const auto& r = rules[i];
        id ^= r.id; left ^= r.left; right ^= r.right; frequency ^= static_cast<std::uint64_t>(r.frequency);
    }
    return AdaptiveRule{static_cast<SymbolId>(id), static_cast<SymbolId>(left), static_cast<SymbolId>(right), static_cast<std::size_t>(frequency)};
}

std::vector<RuleDualParityGroup> build_rule_dual_parity(const std::vector<AdaptiveRule>& rules, std::size_t group_size) {
    std::vector<RuleDualParityGroup> groups;
    if (group_size == 0 || group_size > 255) return groups;
    for (std::size_t begin = 0; begin < rules.size(); begin += group_size) {
        RuleDualParityGroup g;
        g.begin_index = begin;
        g.count = std::min(group_size, rules.size() - begin);
        for (std::size_t j = 0; j < g.count; ++j) {
            const auto& r = rules[begin + j];
            const std::uint8_t c = static_cast<std::uint8_t>(j + 1);
            parity_accumulate(g.p_id, g.q_id, r.id, c);
            parity_accumulate(g.p_left, g.q_left, r.left, c);
            parity_accumulate(g.p_right, g.q_right, r.right, c);
            parity_accumulate(g.p_frequency, g.q_frequency, static_cast<std::uint64_t>(r.frequency), c);
        }
        groups.push_back(g);
    }
    return groups;
}

std::optional<std::pair<AdaptiveRule, AdaptiveRule>> recover_two_rules_from_dual_parity(
    const RuleDualParityGroup& parity,
    const std::vector<AdaptiveRule>& rules,
    std::size_t a_idx,
    std::size_t b_idx) {
    if (a_idx == b_idx || a_idx < parity.begin_index || b_idx < parity.begin_index ||
        a_idx >= parity.begin_index + parity.count || b_idx >= parity.begin_index + parity.count ||
        parity.begin_index + parity.count > rules.size()) return std::nullopt;

    const std::uint8_t ca = static_cast<std::uint8_t>(a_idx - parity.begin_index + 1);
    const std::uint8_t cb = static_cast<std::uint8_t>(b_idx - parity.begin_index + 1);
    if (ca == cb) return std::nullopt;

    auto known_field = [&](auto getter) {
        std::vector<std::pair<std::uint64_t,std::uint8_t>> known;
        for (std::size_t i = parity.begin_index; i < parity.begin_index + parity.count; ++i) {
            if (i == a_idx || i == b_idx) continue;
            known.push_back({getter(rules[i]), static_cast<std::uint8_t>(i - parity.begin_index + 1)});
        }
        return known;
    };

    const auto ids = recover_pair_field(parity.p_id, parity.q_id, ca, cb, known_field([](const AdaptiveRule& r){return static_cast<std::uint64_t>(r.id);}));
    const auto lefts = recover_pair_field(parity.p_left, parity.q_left, ca, cb, known_field([](const AdaptiveRule& r){return static_cast<std::uint64_t>(r.left);}));
    const auto rights = recover_pair_field(parity.p_right, parity.q_right, ca, cb, known_field([](const AdaptiveRule& r){return static_cast<std::uint64_t>(r.right);}));
    const auto freqs = recover_pair_field(parity.p_frequency, parity.q_frequency, ca, cb, known_field([](const AdaptiveRule& r){return static_cast<std::uint64_t>(r.frequency);}));

    AdaptiveRule a{static_cast<SymbolId>(ids.first), static_cast<SymbolId>(lefts.first), static_cast<SymbolId>(rights.first), static_cast<std::size_t>(freqs.first)};
    AdaptiveRule b{static_cast<SymbolId>(ids.second), static_cast<SymbolId>(lefts.second), static_cast<SymbolId>(rights.second), static_cast<std::size_t>(freqs.second)};
    return std::make_pair(a,b);
}

} // namespace bit_analyze
