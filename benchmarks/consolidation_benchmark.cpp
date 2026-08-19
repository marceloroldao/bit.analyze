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
    std::vector<std::vector<bit_analyze::SymbolId>> old_trails;
    old_trails.reserve(corpus.size());

    for (const auto& sample : corpus) {
        online.learn_online(sample, 16, 4, 1.5, 0.001);
        old_trails.push_back(online.encode(sample).trail);
    }

    const auto rules_before = online.rule_count();
    double cost_before = 0.0;
    for (const auto& probe : probes) {
        const auto e = online.encode(probe);
        assert(online.decode(e.trail) == probe);
        cost_before += static_cast<double>(e.trail.size()) / probe.size();
    }
    cost_before /= probes.size();

    const auto learned = online.consolidate(old_trails, 512, 4, 1.5, 0.001);
    const auto rules_after = online.rule_count();

    // Safety property: trails stored before consolidation remain valid.
    for (std::size_t i = 0; i < old_trails.size(); ++i) {
        assert(online.decode(old_trails[i]) == corpus[i]);
    }

    double cost_after = 0.0;
    for (const auto& probe : probes) {
        const auto e = online.encode(probe);
        assert(online.decode(e.trail) == probe);
        cost_after += static_cast<double>(e.trail.size()) / probe.size();
    }
    cost_after /= probes.size();

    bit_analyze::AdaptiveMemory global;
    global.train(corpus, 1024, 4, 1.5, 0.001);
    double global_cost = 0.0;
    for (const auto& probe : probes) {
        const auto e = global.encode(probe);
        assert(global.decode(e.trail) == probe);
        global_cost += static_cast<double>(e.trail.size()) / probe.size();
    }
    global_cost /= probes.size();

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "online_rules_before," << rules_before << '\n';
    std::cout << "consolidation_rules_added," << learned << '\n';
    std::cout << "online_rules_after," << rules_after << '\n';
    std::cout << "global_rules," << global.rule_count() << '\n';
    std::cout << "online_cost_before," << cost_before << '\n';
    std::cout << "online_cost_after," << cost_after << '\n';
    std::cout << "global_cost," << global_cost << '\n';

    return 0;
}
