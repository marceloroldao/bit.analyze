#include "bit_analyze/trail_protection_policy.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace bit_analyze;

    std::vector<AdaptiveRule> rules{
        {256, 'A', 'B', 100},
        {257, 'C', 'D', 80},
        {258, 256, 257, 50}
    };

    std::vector<std::vector<SymbolId>> trails{
        {256, 258, 256, 258, 256},
        {'X','Y','Z','Q','R'},
        {257, 257, 258, 257, 258},
        {'a','b','c'}
    };
    std::vector<std::uint64_t> accesses{1000, 1, 100, 0};

    const auto decisions = assign_trail_protection(trails, accesses, rules, 0.50, 0.75);
    assert(decisions.size() == trails.size());

    std::size_t strong = 0;
    std::size_t medium = 0;
    std::size_t light = 0;
    for (const auto& d : decisions) {
        if (d.profile == ProtectionProfile::Strong) ++strong;
        else if (d.profile == ProtectionProfile::Medium) ++medium;
        else ++light;
    }

    assert(strong >= 1);
    assert(light >= 1);
    assert(decisions[0].criticality > decisions[1].criticality);
    assert(decisions[0].shared_rule_fraction > decisions[1].shared_rule_fraction);

    std::cout << "PASS: adaptive trail protection policy\n";
    std::cout << "light=" << light << " medium=" << medium << " strong=" << strong << '\n';
    return 0;
}
