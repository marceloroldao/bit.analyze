#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/hierarchical_memory.hpp"

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
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

std::vector<std::uint8_t> inject_sparse_noise(std::vector<std::uint8_t> data,
                                              std::size_t period,
                                              std::uint8_t salt) {
    if (period == 0) return data;
    for (std::size_t i = period; i < data.size(); i += period) {
        data[i] = static_cast<std::uint8_t>(data[i] ^ static_cast<std::uint8_t>(salt + (i & 0xFFU)));
    }
    return data;
}

struct Sample {
    std::string name;
    std::vector<std::uint8_t> data;
};

} // namespace

int main() {
    constexpr std::size_t N = 32 * 1024;

    const std::vector<std::uint8_t> A{
        'A','B','C','D','A','B','C','D','0','0','0','0','X','Y','X','Y'
    };
    const std::vector<std::uint8_t> B{
        'm','n','o','p','m','n','o','p','1','2','3','4','1','2','3','4'
    };
    const std::vector<std::uint8_t> C{
        0,1,2,3,0,1,2,3,255,255,0,0,9,8,9,8
    };

    const std::vector<Sample> stream{
        {"A0", repeat_pattern(A, N, 0)},
        {"A3", repeat_pattern(A, N, 3)},
        {"A7_noise", inject_sparse_noise(repeat_pattern(A, N, 7), 257, 11)},
        {"B0", repeat_pattern(B, N, 0)},
        {"B5", repeat_pattern(B, N, 5)},
        {"B9_noise", inject_sparse_noise(repeat_pattern(B, N, 9), 389, 23)},
        {"C0", repeat_pattern(C, N, 0)},
        {"C4", repeat_pattern(C, N, 4)},
        {"C11_noise", inject_sparse_noise(repeat_pattern(C, N, 11), 509, 31)},
        {"A1_again", repeat_pattern(A, N, 1)},
        {"B2_again", repeat_pattern(B, N, 2)},
        {"C6_again", repeat_pattern(C, N, 6)}
    };

    std::vector<std::vector<std::uint8_t>> seen;
    seen.reserve(stream.size());

    std::size_t previous_adaptive_rules = 0;
    std::size_t previous_fixed_relations = 0;

    std::cout
        << "step,name,seen_files,adaptive_rules,adaptive_rule_growth,fixed_relations,fixed_relation_growth,"
        << "probe_adaptive_trail,probe_fixed_trail,probe_fixed_new_relations,adaptive_probe_cost_per_byte,fixed_probe_cost_per_byte\n";

    for (std::size_t step = 0; step < stream.size(); ++step) {
        seen.push_back(stream[step].data);

        bit_analyze::AdaptiveMemory adaptive;
        adaptive.train(seen, 128, 4, 1.5, 0.001);

        bit_analyze::HierarchicalMemory fixed;
        for (const auto& sample : seen) {
            const auto e = fixed.encode(sample, 8);
            assert(fixed.decode(e.trail) == sample);
        }

        const auto adaptive_rules = adaptive.rule_count();
        const auto fixed_relations = fixed.relation_count();
        const auto adaptive_growth = adaptive_rules >= previous_adaptive_rules
            ? adaptive_rules - previous_adaptive_rules : 0;
        const auto fixed_growth = fixed_relations >= previous_fixed_relations
            ? fixed_relations - previous_fixed_relations : 0;

        // Probe with a shifted variant of the latest family. This is never
        // added to the seen corpus before the measurement.
        std::vector<std::uint8_t> probe;
        if (stream[step].name[0] == 'A') probe = repeat_pattern(A, N, (step + 5) % A.size());
        else if (stream[step].name[0] == 'B') probe = repeat_pattern(B, N, (step + 7) % B.size());
        else probe = repeat_pattern(C, N, (step + 9) % C.size());

        const auto adaptive_encoded = adaptive.encode(probe);
        assert(adaptive.decode(adaptive_encoded.trail) == probe);

        const auto fixed_before_probe = fixed.relation_count();
        const auto fixed_encoded = fixed.encode(probe, 8);
        const auto fixed_after_probe = fixed.relation_count();
        assert(fixed.decode(fixed_encoded.trail) == probe);

        const auto fixed_new_on_probe = fixed_after_probe - fixed_before_probe;
        const double n = static_cast<double>(probe.size());
        const double adaptive_probe_cost = adaptive_encoded.trail.size() / n;
        const double fixed_probe_cost = (fixed_encoded.trail.size() + 2.0 * fixed_new_on_probe) / n;

        std::cout
            << (step + 1) << ','
            << stream[step].name << ','
            << seen.size() << ','
            << adaptive_rules << ','
            << adaptive_growth << ','
            << fixed_relations << ','
            << fixed_growth << ','
            << adaptive_encoded.trail.size() << ','
            << fixed_encoded.trail.size() << ','
            << fixed_new_on_probe << ','
            << std::fixed << std::setprecision(6)
            << adaptive_probe_cost << ','
            << fixed_probe_cost << '\n';

        previous_adaptive_rules = adaptive_rules;
        previous_fixed_relations = fixed_relations;
    }

    return 0;
}
