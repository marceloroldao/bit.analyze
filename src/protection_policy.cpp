#include "bit_analyze/protection_policy.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace bit_analyze {

std::unordered_map<SymbolId, std::uint64_t> count_rule_usage(
    const std::vector<std::vector<SymbolId>>& trails,
    const std::vector<AdaptiveRule>& rules) {
    std::unordered_set<SymbolId> known;
    known.reserve(rules.size());
    for (const auto& r : rules) known.insert(r.id);

    std::unordered_map<SymbolId, std::uint64_t> usage;
    usage.reserve(rules.size());
    for (const auto& r : rules) usage[r.id] = 0;

    for (const auto& trail : trails) {
        for (const auto symbol : trail) {
            if (known.find(symbol) != known.end()) ++usage[symbol];
        }
    }
    return usage;
}

namespace {

double quantile(std::vector<double> values, double q) {
    if (values.empty()) return 0.0;
    q = std::clamp(q, 0.0, 1.0);
    std::sort(values.begin(), values.end());
    const double pos = q * static_cast<double>(values.size() - 1);
    const auto lo = static_cast<std::size_t>(std::floor(pos));
    const auto hi = static_cast<std::size_t>(std::ceil(pos));
    if (lo == hi) return values[lo];
    const double t = pos - static_cast<double>(lo);
    return values[lo] * (1.0 - t) + values[hi] * t;
}

} // namespace

std::vector<RuleProtectionDecision> assign_rule_protection(
    const std::vector<AdaptiveRule>& rules,
    const std::unordered_map<SymbolId, std::uint64_t>& usage,
    double medium_quantile,
    double strong_quantile) {
    std::vector<double> scores;
    scores.reserve(rules.size());

    for (const auto& r : rules) {
        const auto it = usage.find(r.id);
        const double u = it == usage.end() ? 0.0 : static_cast<double>(it->second);
        const double f = static_cast<double>(r.frequency);
        // Usage dominates; birth frequency is retained as a weak prior.
        scores.push_back(std::log1p(u) + 0.25 * std::log1p(f));
    }

    const double medium_cut = quantile(scores, medium_quantile);
    const double strong_cut = quantile(scores, strong_quantile);

    std::vector<RuleProtectionDecision> out;
    out.reserve(rules.size());
    for (std::size_t i = 0; i < rules.size(); ++i) {
        const auto& r = rules[i];
        const auto it = usage.find(r.id);
        const auto u = it == usage.end() ? std::uint64_t{0} : it->second;
        const double s = scores[i];
        ProtectionProfile p = ProtectionProfile::Light;
        if (s >= strong_cut && strong_cut > 0.0) p = ProtectionProfile::Strong;
        else if (s >= medium_cut && medium_cut > 0.0) p = ProtectionProfile::Medium;
        out.push_back(RuleProtectionDecision{r.id, u, r.frequency, s, p});
    }
    return out;
}

std::size_t parity_symbols_for_profile(ProtectionProfile profile) noexcept {
    return static_cast<std::size_t>(profile);
}

} // namespace bit_analyze
