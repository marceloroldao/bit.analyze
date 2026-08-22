#include "bit_analyze/trail_protection_policy.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace bit_analyze {

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

std::vector<TrailProtectionDecision> assign_trail_protection(
    const std::vector<std::vector<SymbolId>>& trails,
    const std::vector<std::uint64_t>& access_counts,
    const std::vector<AdaptiveRule>& rules,
    double medium_quantile,
    double strong_quantile) {

    std::unordered_set<SymbolId> rule_ids;
    rule_ids.reserve(rules.size());
    for (const auto& r : rules) rule_ids.insert(r.id);

    std::vector<double> scores;
    std::vector<double> shared;
    scores.reserve(trails.size());
    shared.reserve(trails.size());

    for (std::size_t i = 0; i < trails.size(); ++i) {
        const auto& trail = trails[i];
        std::size_t rule_symbols = 0;
        for (const auto symbol : trail) {
            if (rule_ids.find(symbol) != rule_ids.end()) ++rule_symbols;
        }
        const double shared_fraction = trail.empty()
            ? 0.0
            : static_cast<double>(rule_symbols) / static_cast<double>(trail.size());
        const auto accesses = i < access_counts.size() ? access_counts[i] : std::uint64_t{0};

        // Access frequency dominates. Shared structure and trail size act as
        // secondary signals because damage to a highly shared/long trail can
        // have larger reconstruction impact.
        const double score = std::log1p(static_cast<double>(accesses))
                           + 0.75 * shared_fraction
                           + 0.10 * std::log1p(static_cast<double>(trail.size()));
        shared.push_back(shared_fraction);
        scores.push_back(score);
    }

    const double medium_cut = quantile(scores, medium_quantile);
    const double strong_cut = quantile(scores, strong_quantile);

    std::vector<TrailProtectionDecision> out;
    out.reserve(trails.size());
    for (std::size_t i = 0; i < trails.size(); ++i) {
        ProtectionProfile profile = ProtectionProfile::Light;
        if (scores[i] >= strong_cut && strong_cut > 0.0) profile = ProtectionProfile::Strong;
        else if (scores[i] >= medium_cut && medium_cut > 0.0) profile = ProtectionProfile::Medium;

        out.push_back(TrailProtectionDecision{
            i,
            trails[i].size(),
            i < access_counts.size() ? access_counts[i] : std::uint64_t{0},
            shared[i],
            scores[i],
            profile
        });
    }
    return out;
}

} // namespace bit_analyze
