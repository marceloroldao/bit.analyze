#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/protection_policy.hpp"
#include "bit_analyze/trail_protection_policy.hpp"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

namespace {

struct StrategyResult {
    double preserved_usage{};
    std::size_t parity_cost{};
    std::size_t unrecoverable_groups{};
};

std::size_t capacity(bit_analyze::ProtectionProfile p) {
    return static_cast<std::size_t>(p);
}

StrategyResult simulate_rule_groups(
    const std::vector<bit_analyze::RuleProtectionDecision>& decisions,
    const std::unordered_map<bit_analyze::SymbolId, std::uint64_t>& usage,
    double damage_rate,
    std::mt19937_64& rng) {

    using namespace bit_analyze;
    constexpr std::size_t group_size = 8;

    auto buckets = bucket_rules_by_profile(decisions);
    const double total_usage = [&] {
        double s = 0.0;
        for (const auto& [id, u] : usage) s += static_cast<double>(u);
        return s;
    }();

    double lost_usage = 0.0;
    std::size_t parity = 0;
    std::size_t bad_groups = 0;

    for (const auto& bucket : buckets) {
        const auto cap = capacity(bucket.profile);
        for (std::size_t begin = 0; begin < bucket.rule_ids.size(); begin += group_size) {
            const auto end = std::min(begin + group_size, bucket.rule_ids.size());
            parity += cap;

            std::vector<SymbolId> damaged;
            for (std::size_t i = begin; i < end; ++i) {
                const auto id = bucket.rule_ids[i];
                const double u = static_cast<double>(usage.at(id));
                const double bias = total_usage > 0.0 ? (u / total_usage) : 0.0;
                const double p = std::min(1.0, damage_rate * (1.0 + 25.0 * bias));
                if (std::generate_canonical<double, 53>(rng) < p) damaged.push_back(id);
            }

            if (damaged.size() > cap) {
                ++bad_groups;
                for (const auto id : damaged) lost_usage += static_cast<double>(usage.at(id));
            }
        }
    }

    const double preserved = total_usage > 0.0 ? 1.0 - lost_usage / total_usage : 1.0;
    return StrategyResult{preserved, parity, bad_groups};
}

} // namespace

int main() {
    using namespace bit_analyze;

    // Build a small but heterogeneous synthetic corpus that exercises online learning,
    // consolidation, rule criticality, and trail criticality together.
    AdaptiveMemory memory;
    std::vector<std::vector<std::uint8_t>> corpus;
    for (std::size_t f = 0; f < 24; ++f) {
        std::vector<std::uint8_t> data;
        data.reserve(8192);
        const std::uint8_t a = static_cast<std::uint8_t>('A' + (f % 8));
        const std::uint8_t b = static_cast<std::uint8_t>('a' + (f % 8));
        for (std::size_t i = 0; i < 8192; ++i) {
            if ((i + f) % 11 < 5) data.push_back(a);
            else if ((i + 3 * f) % 17 < 8) data.push_back(b);
            else data.push_back(static_cast<std::uint8_t>((i * 13 + f * 7) & 0xFFU));
        }
        memory.learn_online(data, 24, 3, 1.5, 0.001);
        corpus.push_back(std::move(data));
    }

    std::vector<std::vector<SymbolId>> trails;
    trails.reserve(corpus.size());
    for (const auto& data : corpus) trails.push_back(memory.encode(data).trail);
    memory.consolidate(trails, 128, 3, 1.5, 0.001);
    trails.clear();
    for (const auto& data : corpus) trails.push_back(memory.encode(data).trail);

    const auto usage = count_rule_usage(trails, memory.rules());
    const auto decisions = assign_rule_protection(memory.rules(), usage, 0.80, 0.98);
    const auto buckets = bucket_rules_by_profile(decisions);

    std::size_t light = 0, medium = 0, strong = 0;
    for (const auto& d : decisions) {
        if (d.profile == ProtectionProfile::Light) ++light;
        else if (d.profile == ProtectionProfile::Medium) ++medium;
        else ++strong;
    }

    std::cout << "rules=" << memory.rule_count()
              << " trails=" << trails.size()
              << " light=" << light
              << " medium=" << medium
              << " strong=" << strong << '\n';

    std::mt19937_64 rng(0xB17A11ULL);
    std::cout << "damage,preserved_usage,parity_cost,unrecoverable_groups\n";
    for (double damage : {0.001, 0.01, 0.05, 0.10}) {
        double preserved_sum = 0.0;
        double parity_sum = 0.0;
        double bad_sum = 0.0;
        constexpr std::size_t trials = 1000;
        for (std::size_t t = 0; t < trials; ++t) {
            const auto r = simulate_rule_groups(decisions, usage, damage, rng);
            preserved_sum += r.preserved_usage;
            parity_sum += static_cast<double>(r.parity_cost);
            bad_sum += static_cast<double>(r.unrecoverable_groups);
        }
        std::cout << std::fixed << std::setprecision(4)
                  << damage << ','
                  << (preserved_sum / trials) << ','
                  << (parity_sum / trials) << ','
                  << (bad_sum / trials) << '\n';
    }

    // Lossless invariant remains mandatory after protection-policy analysis.
    for (std::size_t i = 0; i < corpus.size(); ++i) {
        if (memory.decode(trails[i]) != corpus[i]) return 2;
    }

    return 0;
}
