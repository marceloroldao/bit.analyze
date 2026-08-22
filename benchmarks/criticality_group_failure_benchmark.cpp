#include "bit_analyze/protection_policy.hpp"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <unordered_map>
#include <vector>

namespace {

struct Outcome {
    double preserved_usage{};
    std::size_t parity_symbols{};
};

struct Item {
    bit_analyze::RuleProtectionDecision d;
    std::size_t original_index{};
};

Outcome simulate(const std::vector<bit_analyze::RuleProtectionDecision>& decisions,
                 bool adaptive,
                 bit_analyze::ProtectionProfile uniform,
                 double damage_fraction,
                 std::mt19937_64& rng,
                 std::size_t group_size = 8) {
    const std::size_t n = decisions.size();
    const std::size_t damage_count = std::max<std::size_t>(1, static_cast<std::size_t>(n * damage_fraction));

    std::vector<double> weights;
    weights.reserve(n);
    for (const auto& d : decisions) weights.push_back(static_cast<double>(d.usage) + 1.0);
    std::discrete_distribution<std::size_t> pick(weights.begin(), weights.end());

    std::vector<std::uint8_t> damaged(n, 0);
    std::size_t chosen = 0;
    while (chosen < damage_count) {
        const auto i = pick(rng);
        if (!damaged[i]) { damaged[i] = 1; ++chosen; }
    }

    std::vector<std::vector<Item>> buckets(3);
    for (std::size_t i = 0; i < n; ++i) {
        auto p = adaptive ? decisions[i].profile : uniform;
        std::size_t b = p == bit_analyze::ProtectionProfile::Light ? 0 :
                        p == bit_analyze::ProtectionProfile::Medium ? 1 : 2;
        buckets[b].push_back(Item{decisions[i], i});
    }

    std::uint64_t total_usage = 0;
    std::uint64_t lost_usage = 0;
    std::size_t parity = 0;
    for (const auto& d : decisions) total_usage += d.usage;

    for (std::size_t b = 0; b < buckets.size(); ++b) {
        auto p = b == 0 ? bit_analyze::ProtectionProfile::Light :
                 b == 1 ? bit_analyze::ProtectionProfile::Medium :
                          bit_analyze::ProtectionProfile::Strong;
        const std::size_t cap = bit_analyze::parity_symbols_for_profile(p);
        const auto& items = buckets[b];
        for (std::size_t begin = 0; begin < items.size(); begin += group_size) {
            const std::size_t end = std::min(begin + group_size, items.size());
            std::size_t errors = 0;
            std::uint64_t group_usage = 0;
            for (std::size_t i = begin; i < end; ++i) {
                errors += damaged[items[i].original_index] ? 1U : 0U;
                group_usage += items[i].d.usage;
            }
            parity += cap;
            if (errors > cap) lost_usage += group_usage;
        }
    }

    const double preserved = total_usage == 0 ? 1.0 :
        1.0 - static_cast<double>(lost_usage) / static_cast<double>(total_usage);
    return {preserved, parity};
}

} // namespace

int main() {
    using namespace bit_analyze;
    constexpr std::size_t kRules = 1000;
    constexpr std::size_t kTrials = 5000;

    std::vector<AdaptiveRule> rules;
    std::unordered_map<SymbolId, std::uint64_t> usage;
    rules.reserve(kRules);
    for (std::size_t i = 0; i < kRules; ++i) {
        SymbolId id = 256 + i;
        std::uint64_t u = static_cast<std::uint64_t>(1000000.0 / (1.0 + static_cast<double>(i)));
        rules.push_back(AdaptiveRule{id, 0, 0, static_cast<std::size_t>(std::max<std::uint64_t>(2, u / 1000))});
        usage[id] = u;
    }

    const auto decisions = assign_rule_protection(rules, usage, 0.70, 0.95);
    std::mt19937_64 rng(0xC171CULL);

    for (double damage : {0.01, 0.05, 0.10}) {
        double l = 0, a = 0, s = 0;
        double lc = 0, ac = 0, sc = 0;
        for (std::size_t t = 0; t < kTrials; ++t) {
            auto lo = simulate(decisions, false, ProtectionProfile::Light, damage, rng);
            auto ad = simulate(decisions, true, ProtectionProfile::Light, damage, rng);
            auto st = simulate(decisions, false, ProtectionProfile::Strong, damage, rng);
            l += lo.preserved_usage; a += ad.preserved_usage; s += st.preserved_usage;
            lc += lo.parity_symbols; ac += ad.parity_symbols; sc += st.parity_symbols;
        }
        std::cout << std::fixed << std::setprecision(6)
                  << "damage=" << damage
                  << ",light_preserved=" << l / kTrials
                  << ",adaptive_preserved=" << a / kTrials
                  << ",strong_preserved=" << s / kTrials
                  << ",light_parity=" << lc / kTrials
                  << ",adaptive_parity=" << ac / kTrials
                  << ",strong_parity=" << sc / kTrials << '\n';
    }
}
