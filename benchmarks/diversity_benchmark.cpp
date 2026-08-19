#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/hierarchical_memory.hpp"

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
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

std::vector<std::uint8_t> add_noise(std::vector<std::uint8_t> data,
                                    double fraction,
                                    std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> byte_dist(0, 255);
    std::uniform_int_distribution<std::size_t> pos_dist(0, data.size() - 1);
    const auto edits = static_cast<std::size_t>(data.size() * fraction);
    for (std::size_t i = 0; i < edits; ++i) {
        data[pos_dist(rng)] = static_cast<std::uint8_t>(byte_dist(rng));
    }
    return data;
}

std::vector<std::uint8_t> mix_half(const std::vector<std::uint8_t>& a,
                                   const std::vector<std::uint8_t>& b) {
    assert(a.size() == b.size());
    std::vector<std::uint8_t> out;
    out.reserve(a.size());
    const auto mid = a.size() / 2;
    out.insert(out.end(), a.begin(), a.begin() + static_cast<std::ptrdiff_t>(mid));
    out.insert(out.end(), b.begin() + static_cast<std::ptrdiff_t>(mid), b.end());
    return out;
}

std::vector<std::uint8_t> make_random(std::size_t n, std::uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> byte_dist(0, 255);
    std::vector<std::uint8_t> out(n);
    for (auto& b : out) {
        b = static_cast<std::uint8_t>(byte_dist(rng));
    }
    return out;
}

struct Probe {
    std::string name;
    std::vector<std::uint8_t> data;
};

void run_probe(const Probe& probe,
               const bit_analyze::AdaptiveMemory& adaptive,
               bit_analyze::HierarchicalMemory& fixed) {
    const auto adaptive_encoded = adaptive.encode(probe.data);
    assert(adaptive.decode(adaptive_encoded.trail) == probe.data);

    const auto fixed_before = fixed.relation_count();
    const auto fixed_encoded = fixed.encode(probe.data, 8);
    const auto fixed_after = fixed.relation_count();
    assert(fixed.decode(fixed_encoded.trail) == probe.data);

    const auto fixed_new = fixed_after - fixed_before;
    const auto n = static_cast<double>(probe.data.size());
    const double adaptive_cost = adaptive_encoded.trail.size() / n;
    const double fixed_cost = (fixed_encoded.trail.size() + 2.0 * fixed_new) / n;

    std::cout << probe.name << ','
              << probe.data.size() << ','
              << adaptive_encoded.trail.size() << ','
              << fixed_new << ','
              << fixed_encoded.trail.size() << ','
              << std::fixed << std::setprecision(6)
              << adaptive_cost << ','
              << fixed_cost << '\n';
}

} // namespace

int main() {
    constexpr std::size_t N = 64 * 1024;

    const std::vector<std::uint8_t> A{
        'A','B','C','D','A','B','C','D','0','0','0','0','X','Y','X','Y'
    };
    const std::vector<std::uint8_t> B{
        'm','n','o','p','m','n','o','p','1','2','3','4','1','2','3','4'
    };
    const std::vector<std::uint8_t> C{
        0,1,2,3,0,1,2,3,255,255,0,0,9,8,9,8
    };

    std::vector<std::vector<std::uint8_t>> train{
        repeat_pattern(A, N, 0),
        repeat_pattern(B, N, 0),
        repeat_pattern(C, N, 0),
        repeat_pattern(A, N, 3),
        repeat_pattern(B, N, 5),
        repeat_pattern(C, N, 7)
    };

    bit_analyze::AdaptiveMemory adaptive;
    adaptive.train(train, 128, 4, 1.5, 0.001);

    bit_analyze::HierarchicalMemory fixed;
    for (const auto& sample : train) {
        const auto encoded = fixed.encode(sample, 8);
        assert(fixed.decode(encoded.trail) == sample);
    }

    const auto A_shift = repeat_pattern(A, N, 11);
    const auto B_shift = repeat_pattern(B, N, 1);
    const auto C_shift = repeat_pattern(C, N, 4);

    const std::vector<Probe> probes{
        {"shift_A", A_shift},
        {"A_noise_1pct", add_noise(repeat_pattern(A, N, 4), 0.01, 1)},
        {"A_noise_5pct", add_noise(repeat_pattern(A, N, 4), 0.05, 2)},
        {"mix_A_B", mix_half(repeat_pattern(A, N, 2), repeat_pattern(B, N, 6))},
        {"mix_B_C", mix_half(B_shift, C_shift)},
        {"unseen_random", make_random(N, 123)}
    };

    std::cout << "adaptive_rules," << adaptive.rule_count() << '\n';
    std::cout << "probe,bytes,adaptive_trail,fixed_new_relations,fixed_trail,adaptive_cost_per_byte,fixed_cost_per_byte\n";

    for (const auto& probe : probes) {
        run_probe(probe, adaptive, fixed);
    }

    return 0;
}
