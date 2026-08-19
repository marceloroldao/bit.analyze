#include "bit_analyze/adaptive_memory.hpp"

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

std::vector<std::uint8_t> make_family(std::size_t family,
                                      std::size_t n,
                                      std::size_t phase_extra = 0) {
    std::vector<std::uint8_t> motif(16);
    for (std::size_t i = 0; i < motif.size(); ++i) {
        motif[i] = static_cast<std::uint8_t>(
            (family * 37U + i * 11U + (i % 4U) * family) & 0xFFU);
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

double mean_probe_cost(const bit_analyze::AdaptiveMemory& memory,
                       const std::vector<std::vector<std::uint8_t>>& probes) {
    double total = 0.0;
    for (const auto& probe : probes) {
        const auto encoded = memory.encode(probe);
        assert(memory.decode(encoded.trail) == probe);
        total += static_cast<double>(encoded.trail.size()) /
                 static_cast<double>(probe.size());
    }
    return total / static_cast<double>(probes.size());
}

} // namespace

int main() {
    constexpr std::size_t kBytes = 4096;
    constexpr std::size_t kFamiliesPerCycle = 8;
    constexpr std::size_t kCycles = 4;

    bit_analyze::AdaptiveMemory memory;
    std::vector<std::vector<std::uint8_t>> seen;
    std::vector<std::vector<std::uint8_t>> probes;
    std::vector<std::vector<bit_analyze::SymbolId>> stored_trails;

    std::cout << "cycle,seen_families,rules_before_consolidation,meta_rules_added,"
                 "rules_after_consolidation,mean_probe_cost,old_trails_ok\n";

    for (std::size_t cycle = 0; cycle < kCycles; ++cycle) {
        const std::size_t begin_family = cycle * kFamiliesPerCycle;
        const std::size_t end_family = begin_family + kFamiliesPerCycle;

        for (std::size_t f = begin_family; f < end_family; ++f) {
            auto sample = make_family(f, kBytes, 0);
            memory.learn_online(sample, 16, 4, 1.5, 0.001);

            const auto encoded_now = memory.encode(sample);
            assert(memory.decode(encoded_now.trail) == sample);

            seen.push_back(sample);
            probes.push_back(make_family(f, kBytes, 5));
            stored_trails.push_back(encoded_now.trail);
        }

        std::vector<std::vector<bit_analyze::SymbolId>> consolidation_trails;
        consolidation_trails.reserve(seen.size());
        for (const auto& sample : seen) {
            consolidation_trails.push_back(memory.encode(sample).trail);
        }

        const auto before = memory.rule_count();
        const auto meta_added = memory.consolidate(
            consolidation_trails, 64, 4, 1.5, 0.001);
        const auto after = memory.rule_count();

        bool old_trails_ok = true;
        for (std::size_t i = 0; i < stored_trails.size(); ++i) {
            if (memory.decode(stored_trails[i]) != seen[i]) {
                old_trails_ok = false;
                break;
            }
        }
        assert(old_trails_ok);

        const auto cost = mean_probe_cost(memory, probes);

        std::cout << (cycle + 1) << ','
                  << seen.size() << ','
                  << before << ','
                  << meta_added << ','
                  << after << ','
                  << std::fixed << std::setprecision(6)
                  << cost << ','
                  << (old_trails_ok ? 1 : 0) << '\n';
    }

    return 0;
}
