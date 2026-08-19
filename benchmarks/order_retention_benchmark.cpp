#include "bit_analyze/adaptive_memory.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> repeat_pattern(const std::vector<std::uint8_t>& motif,
                                         std::size_t n,
                                         std::size_t phase = 0) {
    std::vector<std::uint8_t> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        out.push_back(motif[(i + phase) % motif.size()]);
    }
    return out;
}

struct Family {
    std::string name;
    std::vector<std::uint8_t> sample;
    std::vector<std::uint8_t> probe;
};

struct RunResult {
    std::size_t rules{};
    double mean_probe_cost{};
    bool all_old_trails_ok{};
};

RunResult run_order(const std::vector<Family>& families,
                    const std::vector<std::size_t>& order) {
    bit_analyze::AdaptiveMemory memory;
    std::vector<std::vector<std::uint8_t>> saved_originals;
    std::vector<std::vector<bit_analyze::SymbolId>> saved_trails;

    bool trails_ok = true;

    for (const auto idx : order) {
        const auto& f = families[idx];

        // Encode before learning this family, then learn it.
        // Old trails must remain valid after every subsequent update.
        const auto before = memory.encode(f.sample);
        saved_originals.push_back(f.sample);
        saved_trails.push_back(before.trail);

        memory.learn_online(f.sample, 24, 4, 1.5, 0.001);

        for (std::size_t i = 0; i < saved_trails.size(); ++i) {
            if (memory.decode(saved_trails[i]) != saved_originals[i]) {
                trails_ok = false;
            }
        }
    }

    double cost_sum = 0.0;
    for (const auto& f : families) {
        const auto enc = memory.encode(f.probe);
        assert(memory.decode(enc.trail) == f.probe);
        cost_sum += static_cast<double>(enc.trail.size()) /
                    static_cast<double>(f.probe.size());
    }

    return RunResult{
        memory.rule_count(),
        cost_sum / static_cast<double>(families.size()),
        trails_ok
    };
}

} // namespace

int main() {
    constexpr std::size_t N = 16 * 1024;

    const std::vector<std::vector<std::uint8_t>> motifs{
        {'A','B','C','D','A','B','C','D','0','0','X','Y','X','Y','0','0'},
        {'m','n','o','p','m','n','o','p','1','2','3','4','1','2','3','4'},
        {0,1,2,3,0,1,2,3,255,255,0,0,9,8,9,8},
        {'q','r','q','r','s','t','s','t','7','7','8','8','9','9','7','7'},
        {'L','M','N','O','L','M','N','O','a','a','b','b','c','c','a','a'},
        {5,5,6,6,5,5,6,6,10,11,10,11,12,13,12,13},
        {'u','v','w','x','u','v','w','x','2','2','4','4','6','6','2','2'},
        {'H','I','J','K','H','I','J','K','r','r','s','s','t','t','r','r'}
    };

    std::vector<Family> families;
    families.reserve(motifs.size());
    for (std::size_t i = 0; i < motifs.size(); ++i) {
        families.push_back(Family{
            "F" + std::to_string(i),
            repeat_pattern(motifs[i], N, i % motifs[i].size()),
            repeat_pattern(motifs[i], N, (i * 3 + 1) % motifs[i].size())
        });
    }

    std::vector<std::vector<std::size_t>> orders;
    std::vector<std::size_t> natural(families.size());
    std::iota(natural.begin(), natural.end(), 0);
    orders.push_back(natural);

    auto reverse = natural;
    std::reverse(reverse.begin(), reverse.end());
    orders.push_back(reverse);

    std::mt19937 rng(12345U);
    for (int i = 0; i < 6; ++i) {
        auto shuffled = natural;
        std::shuffle(shuffled.begin(), shuffled.end(), rng);
        orders.push_back(std::move(shuffled));
    }

    std::cout << "run,rules,mean_probe_cost_per_byte,old_trails_still_valid\n";

    std::size_t min_rules = static_cast<std::size_t>(-1);
    std::size_t max_rules = 0;
    double min_cost = 1e9;
    double max_cost = 0.0;
    bool all_ok = true;

    for (std::size_t i = 0; i < orders.size(); ++i) {
        const auto r = run_order(families, orders[i]);
        min_rules = std::min(min_rules, r.rules);
        max_rules = std::max(max_rules, r.rules);
        min_cost = std::min(min_cost, r.mean_probe_cost);
        max_cost = std::max(max_cost, r.mean_probe_cost);
        all_ok = all_ok && r.all_old_trails_ok;

        std::cout << i << ','
                  << r.rules << ','
                  << std::fixed << std::setprecision(6)
                  << r.mean_probe_cost << ','
                  << (r.all_old_trails_ok ? "yes" : "no") << '\n';
    }

    std::cout << "summary,min_rules=" << min_rules
              << ",max_rules=" << max_rules
              << ",rule_span=" << (max_rules - min_rules)
              << ",min_cost=" << min_cost
              << ",max_cost=" << max_cost
              << ",cost_span=" << (max_cost - min_cost)
              << ",all_old_trails_valid=" << (all_ok ? "yes" : "no")
              << '\n';

    assert(all_ok);
    return 0;
}
