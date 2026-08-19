#include "bit_analyze/adaptive_memory.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

std::vector<std::uint8_t> repeat(const std::vector<std::uint8_t>& motif,
                                 std::size_t n,
                                 std::size_t phase = 0) {
    std::vector<std::uint8_t> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        out.push_back(motif[(i + phase) % motif.size()]);
    }
    return out;
}

} // namespace

int main() {
    using namespace bit_analyze;

    const std::vector<std::uint8_t> A{'A','B','A','B','C','D','C','D'};
    const std::vector<std::uint8_t> B{'x','y','x','y','1','2','1','2'};

    const auto sample_a = repeat(A, 4096, 0);
    const auto sample_b = repeat(B, 4096, 3);
    const auto sample_a_shift = repeat(A, 4096, 5);

    AdaptiveMemory memory;

    const auto learned_a = memory.learn_online(sample_a, 32, 4, 1.5, 0.001);
    assert(learned_a > 0);

    // Store an old trail. It must remain valid forever because online learning
    // is append-only and never renumbers existing symbols.
    const auto old_encoded = memory.encode(sample_a_shift);
    const auto old_trail = old_encoded.trail;
    assert(memory.decode(old_trail) == sample_a_shift);

    const auto rules_before_b = memory.rule_count();
    const auto learned_b = memory.learn_online(sample_b, 32, 4, 1.5, 0.001);
    const auto rules_after_b = memory.rule_count();

    assert(learned_b > 0);
    assert(rules_after_b == rules_before_b + learned_b);

    // Critical invariant: learning B cannot invalidate A's previously stored trail.
    assert(memory.decode(old_trail) == sample_a_shift);

    const auto a_after = memory.encode(sample_a_shift);
    const auto b_after = memory.encode(sample_b);
    assert(memory.decode(a_after.trail) == sample_a_shift);
    assert(memory.decode(b_after.trail) == sample_b);

    std::cout << "PASS: append-only online learning preserves old trails\n";
    std::cout << "rules learned from A: " << learned_a << "\n";
    std::cout << "rules learned from B: " << learned_b << "\n";
    std::cout << "total rules: " << memory.rule_count() << "\n";
    std::cout << "A trail before B: " << old_trail.size() << "\n";
    std::cout << "A trail after B: " << a_after.trail.size() << "\n";
    std::cout << "B trail after learning: " << b_after.trail.size() << "\n";

    return 0;
}
