#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/protection_policy.hpp"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    using namespace bit_analyze;

    std::vector<std::vector<std::uint8_t>> corpus;
    for (std::size_t f = 0; f < 24; ++f) {
        std::vector<std::uint8_t> sample;
        sample.reserve(4096);
        for (std::size_t i = 0; i < 4096; ++i) {
            const auto v = static_cast<std::uint8_t>((f * 29U + (i % 32U) * 7U + ((i / 8U) % 5U) * f) & 0xFFU);
            sample.push_back(v);
        }
        corpus.push_back(std::move(sample));
    }

    AdaptiveMemory memory;
    for (const auto& sample : corpus) memory.learn_online(sample, 12, 3, 1.5, 0.001);

    std::vector<std::vector<SymbolId>> trails;
    for (std::size_t i = 0; i < corpus.size(); ++i) {
        const std::size_t repeats = 1 + (i % 8);
        for (std::size_t r = 0; r < repeats; ++r) trails.push_back(memory.encode(corpus[i]).trail);
    }

    const auto usage = count_rule_usage(trails, memory.rules());
    const auto decisions = assign_rule_protection(memory.rules(), usage, 0.70, 0.95);

    std::size_t adaptive_parity = 0;
    std::size_t light = 0, medium = 0, strong = 0;
    std::uint64_t protected_usage_strong = 0;
    std::uint64_t total_usage = 0;

    for (const auto& d : decisions) {
        adaptive_parity += parity_symbols_for_profile(d.profile);
        total_usage += d.usage;
        if (d.profile == ProtectionProfile::Light) ++light;
        else if (d.profile == ProtectionProfile::Medium) ++medium;
        else {
            ++strong;
            protected_usage_strong += d.usage;
        }
    }

    const std::size_t uniform_strong_parity = decisions.size() * 6;
    const std::size_t uniform_light_parity = decisions.size() * 2;

    const double saving_vs_strong = uniform_strong_parity == 0 ? 0.0 :
        1.0 - static_cast<double>(adaptive_parity) / static_cast<double>(uniform_strong_parity);
    const double strong_usage_share = total_usage == 0 ? 0.0 :
        static_cast<double>(protected_usage_strong) / static_cast<double>(total_usage);

    std::cout << "rules," << decisions.size() << '\n';
    std::cout << "light," << light << '\n';
    std::cout << "medium," << medium << '\n';
    std::cout << "strong," << strong << '\n';
    std::cout << "uniform_light_parity_symbols," << uniform_light_parity << '\n';
    std::cout << "adaptive_parity_symbols," << adaptive_parity << '\n';
    std::cout << "uniform_strong_parity_symbols," << uniform_strong_parity << '\n';
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "saving_vs_uniform_strong," << saving_vs_strong << '\n';
    std::cout << "usage_share_in_strong," << strong_usage_share << '\n';
    return 0;
}
