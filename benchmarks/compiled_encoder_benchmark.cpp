#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/compiled_encoder.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

using Clock = std::chrono::steady_clock;

namespace {

std::vector<std::uint8_t> make_pattern(std::size_t n, std::size_t phase) {
    static const std::vector<std::uint8_t> motif{
        'A','B','C','D','A','B','C','D','0','0','0','0','X','Y','X','Y'
    };
    std::vector<std::uint8_t> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        out.push_back(motif[(i + phase) % motif.size()]);
    }
    return out;
}

template <class F>
long long micros(F&& fn) {
    const auto t0 = Clock::now();
    fn();
    const auto t1 = Clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}

} // namespace

int main() {
    using namespace bit_analyze;

    constexpr std::size_t kTrainBytes = 32 * 1024;
    constexpr std::size_t kProbeBytes = 512 * 1024;

    AdaptiveMemory memory;
    for (std::size_t i = 0; i < 12; ++i) {
        memory.learn_online(make_pattern(kTrainBytes, i % 16), 16, 4, 1.5, 0.001);
    }

    const auto probe = make_pattern(kProbeBytes, 5);

    AdaptiveEncodeResult canonical;
    const auto canonical_us = micros([&] { canonical = memory.encode(probe); });
    assert(memory.decode(canonical.trail) == probe);

    CompiledEncoder compiled(memory);
    CompiledEncodeResult fast;
    const auto compiled_us = micros([&] { fast = compiled.encode(probe); });
    assert(memory.decode(fast.trail) == probe);

    const double speedup = compiled_us > 0
        ? static_cast<double>(canonical_us) / static_cast<double>(compiled_us)
        : 0.0;

    std::cout << "rules," << memory.rule_count() << '\n';
    std::cout << "trie_nodes," << compiled.trie_node_count() << '\n';
    std::cout << "probe_bytes," << probe.size() << '\n';
    std::cout << "canonical_trail," << canonical.trail.size() << '\n';
    std::cout << "compiled_trail," << fast.trail.size() << '\n';
    std::cout << "canonical_us," << canonical_us << '\n';
    std::cout << "compiled_us," << compiled_us << '\n';
    std::cout << std::fixed << std::setprecision(3)
              << "speedup," << speedup << '\n';

    return 0;
}
