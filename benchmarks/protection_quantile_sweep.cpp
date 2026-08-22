#include "bit_analyze/protection_policy.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

namespace {

struct Eval {
    double preserved_usage{};
    double mean_parity{};
    double efficiency{};
};

Eval evaluate(const std::vector<bit_analyze::RuleProtectionDecision>& decisions,
              double damage_rate,
              std::uint64_t seed) {
    using namespace bit_analyze;
    constexpr std::size_t kGroup = 8;
    constexpr std::size_t kTrials = 1500;

    std::vector<RuleProtectionDecision> ordered = decisions;
    std::stable_sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
        if (a.profile != b.profile) return static_cast<int>(a.profile) < static_cast<int>(b.profile);
        return a.criticality > b.criticality;
    });

    double total_usage = 0.0;
    for (const auto& d : ordered) total_usage += static_cast<double>(d.usage);

    std::size_t parity = 0;
    for (std::size_t begin = 0; begin < ordered.size();) {
        const auto profile = ordered[begin].profile;
        std::size_t end = begin;
        while (end < ordered.size() && ordered[end].profile == profile && end - begin < kGroup) ++end;
        parity += parity_symbols_for_profile(profile);
        begin = end;
    }

    std::mt19937_64 rng(seed);
    std::vector<double> weights;
    weights.reserve(ordered.size());
    for (const auto& d : ordered) weights.push_back(1.0 + std::log1p(static_cast<double>(d.usage)));
    std::discrete_distribution<std::size_t> pick(weights.begin(), weights.end());

    double preserved_sum = 0.0;
    const std::size_t damage_count = std::max<std::size_t>(1, static_cast<std::size_t>(std::round(damage_rate * ordered.size())));

    for (std::size_t trial = 0; trial < kTrials; ++trial) {
        std::vector<bool> damaged(ordered.size(), false);
        std::size_t selected = 0;
        while (selected < damage_count) {
            const auto i = pick(rng);
            if (!damaged[i]) { damaged[i] = true; ++selected; }
        }

        double preserved = total_usage;
        for (std::size_t begin = 0; begin < ordered.size();) {
            const auto profile = ordered[begin].profile;
            std::size_t end = begin;
            while (end < ordered.size() && ordered[end].profile == profile && end - begin < kGroup) ++end;
            std::size_t bad = 0;
            for (std::size_t i = begin; i < end; ++i) if (damaged[i]) ++bad;
            if (bad > parity_symbols_for_profile(profile)) {
                for (std::size_t i = begin; i < end; ++i) preserved -= static_cast<double>(ordered[i].usage);
            }
            begin = end;
        }
        preserved_sum += preserved / total_usage;
    }

    const double preserved = preserved_sum / static_cast<double>(kTrials);
    const double mean_parity = static_cast<double>(parity) / static_cast<double>(ordered.size());
    const double efficiency = preserved / mean_parity;
    return {preserved, mean_parity, efficiency};
}

} // namespace

int main() {
    using namespace bit_analyze;
    constexpr std::size_t kRules = 1000;

    std::vector<AdaptiveRule> rules;
    std::unordered_map<SymbolId, std::uint64_t> usage;
    rules.reserve(kRules);
    for (std::size_t i = 0; i < kRules; ++i) {
        const SymbolId id = 256 + i;
        const std::uint64_t u = static_cast<std::uint64_t>(1000000.0 / (1.0 + static_cast<double>(i)));
        rules.push_back(AdaptiveRule{id, 0, 0, static_cast<std::size_t>(std::max<std::uint64_t>(2, u / 1000))});
        usage[id] = u;
    }

    std::cout << "medium_q,strong_q,preserved_1pct,preserved_5pct,preserved_10pct,mean_parity,score\n";
    for (double mq : {0.50, 0.60, 0.70, 0.80}) {
        for (double sq : {0.85, 0.90, 0.95, 0.98}) {
            if (sq <= mq) continue;
            const auto d = assign_rule_protection(rules, usage, mq, sq);
            const auto e1 = evaluate(d, 0.01, 1001);
            const auto e5 = evaluate(d, 0.05, 1005);
            const auto e10 = evaluate(d, 0.10, 1010);
            const double score = (0.5 * e1.preserved_usage + 0.3 * e5.preserved_usage + 0.2 * e10.preserved_usage) / e1.mean_parity;
            std::cout << std::fixed << std::setprecision(2) << mq << ',' << sq << ','
                      << std::setprecision(6) << e1.preserved_usage << ',' << e5.preserved_usage << ','
                      << e10.preserved_usage << ',' << e1.mean_parity << ',' << score << '\n';
        }
    }
    return 0;
}
