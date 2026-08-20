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
constexpr std::size_t kCapacity = 2;
constexpr std::size_t kTrials = 5000;

std::size_t contiguous_group(std::size_t index) {
    return index / kGroupSize;
}

std::size_t interleaved_group(std::size_t index) {
    return index % kGroups;
}

template <typename Mapper>
bool recoverable(const std::vector<std::size_t>& damaged, Mapper mapper) {
    std::array<std::size_t, kGroups> counts{};
    for (const auto index : damaged) {
        const auto g = mapper(index);
        if (++counts[g] > kCapacity) return false;
    }
    return true;
}

template <typename Mapper>
double random_failure_probability(std::size_t errors, Mapper mapper, std::mt19937_64& rng) {
    std::vector<std::size_t> indices(kSymbols);
    std::iota(indices.begin(), indices.end(), 0);
    std::size_t failures = 0;

    for (std::size_t trial = 0; trial < kTrials; ++trial) {
        std::shuffle(indices.begin(), indices.end(), rng);
        std::vector<std::size_t> damaged(indices.begin(), indices.begin() + errors);
        if (!recoverable(damaged, mapper)) ++failures;
    }
    return static_cast<double>(failures) / static_cast<double>(kTrials);
}

template <typename Mapper>
double burst_failure_probability(std::size_t errors, Mapper mapper) {
    if (errors == 0 || errors > kSymbols) return 0.0;
    const auto starts = kSymbols - errors + 1;
    std::size_t failures = 0;
    std::vector<std::size_t> damaged(errors);

    for (std::size_t start = 0; start < starts; ++start) {
        for (std::size_t i = 0; i < errors; ++i) damaged[i] = start + i;
        if (!recoverable(damaged, mapper)) ++failures;
    }
    return static_cast<double>(failures) / static_cast<double>(starts);
}

} // namespace

int main() {
    const std::array<double, 3> rates{0.0001, 0.001, 0.01};
    std::mt19937_64 rng(0xB17A4AULL);

    std::cout << "symbols," << kSymbols << '\n';
    std::cout << "groups," << kGroups << '\n';
    std::cout << "capacity_per_group," << kCapacity << '\n';
    std::cout << "trials," << kTrials << '\n';
    std::cout << "rate,errors,mode,layout,failure_probability\n";
    std::cout << std::fixed << std::setprecision(6);

    for (const auto rate : rates) {
        const auto errors = std::max<std::size_t>(1, static_cast<std::size_t>(kSymbols * rate + 0.5));

        const auto rc = random_failure_probability(errors, contiguous_group, rng);
        const auto ri = random_failure_probability(errors, interleaved_group, rng);
        const auto bc = burst_failure_probability(errors, contiguous_group);
        const auto bi = burst_failure_probability(errors, interleaved_group);

        std::cout << rate << ',' << errors << ",random,contiguous," << rc << '\n';
        std::cout << rate << ',' << errors << ",random,interleaved," << ri << '\n';
        std::cout << rate << ',' << errors << ",burst,contiguous," << bc << '\n';
        std::cout << rate << ',' << errors << ",burst,interleaved," << bi << '\n';
    }

    return 0;
}
