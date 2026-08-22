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

struct SimResult {
    double preserved_usage{};
    double parity_cost{};
};

std::size_t capacity_for(bit_analyze::ProtectionProfile p) {
    return bit_analyze::parity_symbols_for_profile(p);
}

SimResult run_strategy(
    const std::vector<bit_analyze::RuleProtectionDecision>& decisions,
    bit_analyze::ProtectionProfile uniform_profile,
    bool adaptive,
    std::mt19937_64& rng,
    double damage_fraction) {

    const std::size_t n = decisions.size();
    const std::size_t damage_count = std::max<std::size_t>(1, static_cast<std::size_t>(n * damage_fraction));

    std::vector<double> weights;
    weights.reserve(n);
    for (const auto& d : decisions) weights.push_back(static_cast<double>(d.usage) + 1.0);

    std::discrete_distribution<std::size_t> pick(weights.begin(), weights.end());
    std::vector<std::uint8_t> hits(n, 0);
    std::size_t chosen = 0;
    while (chosen < damage_count) {
        const auto i = pick(rng);
        if (hits[i] == 0) {
            hits[i] = 1;
            ++chosen;
        }
    }

    std::uint64_t total_usage = 0;
    std::uint64_t lost_usage = 0;
    double parity_cost = 0.0;

    for (std::size_t i = 0; i < n; ++i) {
        const auto profile = adaptive ? decisions[i].profile : uniform_profile;
        const auto cap = capacity_for(profile);
        parity_cost += static_cast<double>(cap);
        total_usage += decisions[i].usage;
        if (hits[i] && cap < 4) {
            lost_usage += decisions[i].usage;
        }
    }

    const double preserved = total_usage == 0 ? 1.0 :
        1.0 - static_cast<double>(lost_usage) / static_cast<double>(total_usage);
    return {preserved, parity_cost};
}

} // namespace

int main() {
    using namespace bit_analyze;

    constexpr std::size_t kRules = 1000;
    constexpr std::size_t kTrials = 5000;

    std::vector<AdaptiveRule> rules;
    rules.reserve(kRules);
    std::unordered_map<SymbolId, std::uint64_t> usage;

    for (std::size_t i = 0; i < kRules; ++i) {
        const SymbolId id = 256 + i;
        const std::uint64_t u = static_cast<std::uint64_t>(1000000.0 / (1.0 + static_cast<double>(i)));
        rules.push_back(AdaptiveRule{id, 0, 0, static_cast<std::size_t>(std::max<std::uint64_t>(2, u / 1000))});
        usage[id] = u;
    }

    const auto decisions = assign_rule_protection(rules, usage, 0.70, 0.95);

    std::mt19937_64 rng(0xB17AULL);
    for (double damage : {0.01, 0.05, 0.10}) {
        double light_pres = 0.0;
        double adapt_pres = 0.0;
        double strong_pres = 0.0;
        double light_cost = 0.0;
        double adapt_cost = 0.0;
        double strong_cost = 0.0;

        for (std::size_t t = 0; t < kTrials; ++t) {
            auto a = run_strategy(decisions, ProtectionProfile::Light, false, rng, damage);
            auto b = run_strategy(decisions, ProtectionProfile::Light, true, rng, damage);
            auto c = run_strategy(decisions, ProtectionProfile::Strong, false, rng, damage);
            light_pres += a.preserved_usage; adapt_pres += b.preserved_usage; strong_pres += c.preserved_usage;
            light_cost += a.parity_cost; adapt_cost += b.parity_cost; strong_cost += c.parity_cost;
        }

        std::cout << std::fixed << std::setprecision(6)
                  << "damage=" << damage
                  << ",light_preserved=" << light_pres / kTrials
                  << ",adaptive_preserved=" << adapt_pres / kTrials
                  << ",strong_preserved=" << strong_pres / kTrials
                  << ",light_cost=" << light_cost / kTrials
                  << ",adaptive_cost=" << adapt_cost / kTrials
                  << ",strong_cost=" << strong_cost / kTrials
                  << '\n';
    }

    return 0;
}
