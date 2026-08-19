#include "bit_analyze/adaptive_memory.hpp"

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

std::vector<std::uint8_t> make_family(std::size_t family, std::size_t n, std::size_t phase_extra = 0) {
    std::vector<std::uint8_t> motif(16);
    for (std::size_t i = 0; i < motif.size(); ++i) {
        motif[i] = static_cast<std::uint8_t>((family * 37U + i * 11U + (i % 4U) * family) & 0xFFU);
    }
    motif[4] = motif[0];
    motif[5] = motif[1];
    motif[12] = motif[8];
    motif[13] = motif[9];

    std::vector<std::uint8_t> out;
    out.reserve(n);
    const auto phase = (family * 3U + phase_extra) % motif.size();
    for (std::size_t i = 0; i < n; ++i) {
        out.push_back(motif[(i + phase) % motif.size()]);
    }
    return out;
}

} // namespace

int main() {
    constexpr std::size_t kFamilies = 32;
    constexpr std::size_t kBytes = 4096;

    std::vector<std::vector<std::uint8_t>> corpus;
    std::vector<std::vector<std::uint8_t>> probes;
    corpus.reserve(kFamilies);
    probes.reserve(kFamilies);

    for (std::size_t f = 0; f < kFamilies; ++f) {
        corpus.push_back(make_family(f, kBytes, 0));
        probes.push_back(make_family(f, kBytes, 5));
    }

    bit_analyze::AdaptiveMemory online;
    for (const auto& sample : corpus) {
        online.learn_online(sample, 16, 4, 1.5, 0.001);
    }

    // Use a generous cap so the global learner reaches structural saturation
    // instead of looking artificially worse because of a rule budget.
    bit_analyze::AdaptiveMemory global;
    global.train(corpus, 1024, 4, 1.5, 0.001);

    double online_cost = 0.0;
    double global_cost = 0.0;

    for (const auto& probe : probes) {
        const auto oe = online.encode(probe);
        const auto ge = global.encode(probe);
        assert(online.decode(oe.trail) == probe);
        assert(global.decode(ge.trail) == probe);

        online_cost += static_cast<double>(oe.trail.size()) / probe.size();
        global_cost += static_cast<double>(ge.trail.size()) / probe.size();
    }

    online_cost /= probes.size();
    global_cost /= probes.size();

    std::cout << "online_rules," << online.rule_count() << '\n';
    std::cout << "global_rules," << global.rule_count() << '\n';
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "online_mean_probe_cost," << online_cost << '\n';
    std::cout << "global_mean_probe_cost," << global_cost << '\n';
    std::cout << "rule_ratio_online_over_global,"
              << static_cast<double>(online.rule_count()) /
                 static_cast<double>(global.rule_count()) << '\n';
    std::cout << "probe_cost_ratio_online_over_global,"
              << online_cost / global_cost << '\n';

    return 0;
}
