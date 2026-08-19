#include "bit_analyze/adaptive_memory.hpp"

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

    const std::vector<std::size_t> capacities{64, 128, 256, 512};

    std::cout << "capacity,step,name,seen_files,rules,at_capacity,probe_trail,probe_cost_per_byte\n";

    for (const auto capacity : capacities) {
        std::vector<std::vector<std::uint8_t>> seen;
        seen.reserve(stream.size());

        for (std::size_t step = 0; step < stream.size(); ++step) {
            seen.push_back(stream[step].data);

            bit_analyze::AdaptiveMemory adaptive;
            adaptive.train(seen, capacity, 4, 1.5, 0.001);

            const auto family = stream[step].name[0];
            const auto& motif = family == 'A' ? A : (family == 'B' ? B : C);
            const auto phase = (step * 3 + 5) % motif.size();
            const auto probe = repeat_pattern(motif, N, phase);

            const auto encoded = adaptive.encode(probe);
            assert(adaptive.decode(encoded.trail) == probe);

            std::cout
                << capacity << ','
                << (step + 1) << ','
                << stream[step].name << ','
                << seen.size() << ','
                << adaptive.rule_count() << ','
                << (adaptive.rule_count() == capacity ? 1 : 0) << ','
                << encoded.trail.size() << ','
                << std::fixed << std::setprecision(6)
                << (static_cast<double>(encoded.trail.size()) / static_cast<double>(probe.size()))
                << '\n';
        }
    }

    return 0;
}
