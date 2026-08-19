#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/integrity.hpp"
#include "bit_analyze/recovery.hpp"
#include "bit_analyze/trail_recovery.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

double pct(std::size_t overhead, std::size_t payload) {
    return payload == 0 ? 0.0 : 100.0 * static_cast<double>(overhead) /
                                   static_cast<double>(payload);
}

} // namespace

int main() {
    using namespace bit_analyze;

    constexpr std::size_t kRuleGroup = 8;
    constexpr std::size_t kTrailBlock = 64;
    constexpr std::size_t kRules = 1024;
    constexpr std::size_t kTrailSymbols = 1024 * 1024;

    const std::size_t rule_payload = kRules * sizeof(AdaptiveRule);
    const std::size_t rule_group_count = (kRules + kRuleGroup - 1) / kRuleGroup;
    const std::size_t rule_dual_parity = rule_group_count * sizeof(RuleDualParityGroup);
    const std::size_t rule_hashes = kRules * sizeof(std::uint64_t);

    const std::size_t trail_payload = kTrailSymbols * sizeof(SymbolId);
    const std::size_t trail_block_count = (kTrailSymbols + kTrailBlock - 1) / kTrailBlock;
    const std::size_t trail_dual_parity = trail_block_count * sizeof(TrailDualParityBlock);
    const std::size_t trail_block_hashes = trail_block_count * sizeof(std::uint64_t);
    const std::size_t trail_global_hash = sizeof(std::uint64_t);

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "sizeof_adaptive_rule," << sizeof(AdaptiveRule) << '\n';
    std::cout << "sizeof_rule_dual_parity_group," << sizeof(RuleDualParityGroup) << '\n';
    std::cout << "sizeof_trail_dual_parity_block," << sizeof(TrailDualParityBlock) << '\n';

    std::cout << "rule_payload_bytes," << rule_payload << '\n';
    std::cout << "rule_parity_bytes," << rule_dual_parity << '\n';
    std::cout << "rule_hash_bytes," << rule_hashes << '\n';
    std::cout << "rule_parity_overhead_pct," << pct(rule_dual_parity, rule_payload) << '\n';
    std::cout << "rule_integrity_overhead_pct," << pct(rule_hashes, rule_payload) << '\n';
    std::cout << "rule_total_protection_overhead_pct,"
              << pct(rule_dual_parity + rule_hashes, rule_payload) << '\n';

    std::cout << "trail_payload_bytes," << trail_payload << '\n';
    std::cout << "trail_parity_bytes," << trail_dual_parity << '\n';
    std::cout << "trail_block_hash_bytes," << trail_block_hashes << '\n';
    std::cout << "trail_parity_overhead_pct," << pct(trail_dual_parity, trail_payload) << '\n';
    std::cout << "trail_integrity_overhead_pct,"
              << pct(trail_block_hashes + trail_global_hash, trail_payload) << '\n';
    std::cout << "trail_total_protection_overhead_pct,"
              << pct(trail_dual_parity + trail_block_hashes + trail_global_hash,
                     trail_payload) << '\n';

    const std::size_t combined_payload = rule_payload + trail_payload;
    const std::size_t combined_overhead = rule_dual_parity + rule_hashes +
                                          trail_dual_parity + trail_block_hashes +
                                          trail_global_hash;
    std::cout << "combined_overhead_pct," << pct(combined_overhead, combined_payload) << '\n';

    return 0;
}
