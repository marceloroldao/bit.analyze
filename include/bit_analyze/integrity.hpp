#pragma once

#include "bit_analyze/adaptive_memory.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bit_analyze {

struct IntegrityManifest {
    std::vector<std::uint64_t> rule_hashes;
    std::uint64_t trail_hash{};
};

std::uint64_t hash_rule(const AdaptiveRule& rule) noexcept;
std::uint64_t hash_trail(const std::vector<SymbolId>& trail) noexcept;

IntegrityManifest build_integrity_manifest(
    const std::vector<AdaptiveRule>& rules,
    const std::vector<SymbolId>& trail);

std::vector<std::size_t> find_corrupted_rules(
    const IntegrityManifest& manifest,
    const std::vector<AdaptiveRule>& rules);

bool verify_trail(
    const IntegrityManifest& manifest,
    const std::vector<SymbolId>& trail) noexcept;

} // namespace bit_analyze
