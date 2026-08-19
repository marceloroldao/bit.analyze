#include "bit_analyze/adaptive_memory.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

std::vector<std::uint8_t> make_family(std::size_t family, std::size_t n) {
    std::vector<std::uint8_t> motif(16);
    for (std::size_t i = 0; i < motif.size(); ++i) {
        motif[i] = static_cast<std::uint8_t>((family * 37U + i * 11U + (i % 4U) * family) & 0xFFU);
    }
    // Add internal recurrence shared within the family.
    motif[4] = motif[0];
    motif[5] = motif[1];
    motif[12] = motif[8];
    motif[13] = motif[9];

    std::vector<std::uint8_t> out;
    out.reserve(n);
    const auto phase = (family * 3U) % motif.size();
    for (std::size_t i = 0; i < n; ++i) {
        out.push_back(motif[(i + phase) % motif.size()]);
    }
    return out;
}

} // namespace

int main() {
    constexpr std::size_t kFamilies = 32;
    constexpr std::size_t kBytes = 4096;

    bit_analyze::AdaptiveMemory memory;
    std::vector<std::vector<std::uint8_t>> originals;
    std::vector<std::vector<bit_analyze::SymbolId>> old_trails;

    std::size_t previous_rules = 0;

    for (std::size_t family = 0; family < kFamilies; ++family) {
        auto sample = make_family(family, kBytes);

        const auto before_learning = memory.encode(sample);
        originals.push_back(sample);
        old_trails.push_back(before_learning.trail);

        const auto learned = memory.learn_online(sample, 16, 4, 1.5, 0.001);
        const auto now_rules = memory.rule_count();

        assert(now_rules >= previous_rules);
        assert(now_rules == previous_rules + learned);
        previous_rules = now_rules;

        // Every historical trail must decode exactly after every update.
        for (std::size_t i = 0; i < old_trails.size(); ++i) {
            assert(memory.decode(old_trails[i]) == originals[i]);
        }

        std::cout << "family=" << family
                  << ",learned=" << learned
                  << ",rules=" << now_rules
                  << '\n';
    }

    // Final retention pass after all 32 families.
    for (std::size_t i = 0; i < old_trails.size(); ++i) {
        assert(memory.decode(old_trails[i]) == originals[i]);
    }

    std::cout << "PASS: 32-family online retention stress\n";
    std::cout << "Final rules: " << memory.rule_count() << '\n';
    return 0;
}
