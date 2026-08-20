#include <algorithm>
#include <array>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

namespace {
constexpr std::size_t kSymbols = 4096;
constexpr std::size_t kGroups = 64;
constexpr std::size_t kGroupSize = kSymbols / kGroups;
constexpr std::size_t kTrials = 5000;
constexpr double kRandomRate = 0.01; // 1%

std::size_t group_of(std::size_t index) { return index % kGroups; }

double failure_probability(std::size_t errors,
                           std::size_t capacity,
                           std::mt19937_64& rng) {
    std::vector<std::size_t> indices(kSymbols);
    std::iota(indices.begin(), indices.end(), 0);
    std::size_t failures = 0;

    for (std::size_t t = 0; t < kTrials; ++t) {
        std::shuffle(indices.begin(), indices.end(), rng);
        std::array<std::size_t, kGroups> counts{};
        bool failed = false;
        for (std::size_t i = 0; i < errors; ++i) {
            const auto g = group_of(indices[i]);
            if (++counts[g] > capacity) {
                failed = true;
                break;
            }
        }
        if (failed) ++failures;
    }

    return static_cast<double>(failures) / static_cast<double>(kTrials);
}

} // namespace

int main() {
    const auto errors = static_cast<std::size_t>(kSymbols * kRandomRate + 0.5);
    const std::array<std::size_t, 5> capacities{2, 3, 4, 5, 6};
    std::mt19937_64 rng(0xC0FFEEULL);

    std::cout << "symbols," << kSymbols << '\n';
    std::cout << "groups," << kGroups << '\n';
    std::cout << "group_size," << kGroupSize << '\n';
    std::cout << "random_rate," << kRandomRate << '\n';
    std::cout << "errors," << errors << '\n';
    std::cout << "capacity,parity_symbols_per_group,theoretical_trail_overhead,failure_probability\n";
    std::cout << std::fixed << std::setprecision(6);

    for (const auto capacity : capacities) {
        // A generic erasure code capable of recovering `capacity` symbol erasures
        // requires at least that many independent parity symbols per group.
        const double overhead = static_cast<double>(capacity) /
                                static_cast<double>(kGroupSize);
        const double fail = failure_probability(errors, capacity, rng);
        std::cout << capacity << ',' << capacity << ',' << overhead << ',' << fail << '\n';
    }
    return 0;
}
