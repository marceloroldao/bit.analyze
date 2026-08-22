#include "bit_analyze/protection_policy.hpp"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

int main() {
    using namespace bit_analyze;

    constexpr std::size_t kRules = 1000;
    std::vector<AdaptiveRule> rules;
    std::unordered_map<SymbolId, std::uint64_t> usage;
    rules.reserve(kRules);

    for (std::size_t i = 0; i < kRules; ++i) {
        const SymbolId id = 256 + i;
        const std::uint64_t u = static_cast<std::uint64_t>(1000000.0 / (1.0 + static_cast<double>(i)));
        rules.push_back(AdaptiveRule{id, 0, 0, static_cast<std::size_t>(std::max<std::uint64_t>(2, u / 1000))});
        usage[id] = u;
    }

    std::cout << "medium_q,strong_q,light,medium,strong,mean_parity_per_rule\n";
    for (double mq : {0.50, 0.60, 0.70, 0.80}) {
        for (double sq : {0.85, 0.90, 0.95, 0.98}) {
            if (sq <= mq) continue;
            const auto decisions = assign_rule_protection(rules, usage, mq, sq);
            std::size_t l=0,m=0,s=0;
            std::size_t parity=0;
            for (const auto& d : decisions) {
                if (d.profile == ProtectionProfile::Light) ++l;
                else if (d.profile == ProtectionProfile::Medium) ++m;
                else ++s;
                parity += parity_symbols_for_profile(d.profile);
            }
            std::cout << std::fixed << std::setprecision(2)
                      << mq << ',' << sq << ',' << l << ',' << m << ',' << s << ','
                      << std::setprecision(6)
                      << static_cast<double>(parity) / static_cast<double>(decisions.size()) << '\n';
        }
    }
}
