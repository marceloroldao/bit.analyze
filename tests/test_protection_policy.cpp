#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/protection_policy.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace bit_analyze;

    std::vector<std::uint8_t> a;
    std::vector<std::uint8_t> b;
    const std::vector<std::uint8_t> ma{'A','B','A','B','A','B','C','D'};
    const std::vector<std::uint8_t> mb{'X','Y','X','Y','0','0','0','0'};
    for (std::size_t i = 0; i < 8192; ++i) {
        a.push_back(ma[i % ma.size()]);
        b.push_back(mb[i % mb.size()]);
    }

    AdaptiveMemory memory;
    memory.learn_online(a, 24, 3, 1.5, 0.001);
    memory.learn_online(b, 24, 3, 1.5, 0.001);

    std::vector<std::vector<SymbolId>> trails;
    for (int i = 0; i < 10; ++i) trails.push_back(memory.encode(a).trail);
    trails.push_back(memory.encode(b).trail);

    const auto usage = count_rule_usage(trails, memory.rules());
    const auto decisions = assign_rule_protection(memory.rules(), usage, 0.70, 0.95);
    assert(decisions.size() == memory.rules().size());

    std::size_t light = 0, medium = 0, strong = 0;
    std::uint64_t strong_usage = 0;
    std::uint64_t total_usage = 0;
    for (const auto& d : decisions) {
        total_usage += d.usage;
        if (d.profile == ProtectionProfile::Light) ++light;
        else if (d.profile == ProtectionProfile::Medium) ++medium;
        else {
            ++strong;
            strong_usage += d.usage;
        }
    }

    assert(light + medium + strong == decisions.size());
    assert(strong > 0);
    assert(total_usage == 0 || strong_usage <= total_usage);

    std::cout << "PASS: adaptive protection policy\n";
    std::cout << "rules=" << decisions.size()
              << " light=" << light
              << " medium=" << medium
              << " strong=" << strong << '\n';
    return 0;
}
