#include "bit_analyze/integrity.hpp"

#include <algorithm>

namespace bit_analyze {

namespace {

constexpr std::uint64_t kOffset = 1469598103934665603ULL;
constexpr std::uint64_t kPrime  = 1099511628211ULL;

inline void mix_u64(std::uint64_t& h, std::uint64_t v) noexcept {
    for (int i = 0; i < 8; ++i) {
        h ^= static_cast<std::uint8_t>((v >> (i * 8)) & 0xFFU);
        h *= kPrime;
    }
}

std::uint64_t hash_trail_range(const std::vector<SymbolId>& trail,
                               std::size_t begin,
                               std::size_t end) noexcept {
    std::uint64_t h = kOffset;
    mix_u64(h, static_cast<std::uint64_t>(end - begin));
    for (std::size_t i = begin; i < end; ++i) mix_u64(h, trail[i]);
    return h;
}

} // namespace

std::uint64_t hash_rule(const AdaptiveRule& rule) noexcept {
    std::uint64_t h = kOffset;
    mix_u64(h, rule.id);
    mix_u64(h, rule.left);
    mix_u64(h, rule.right);
    mix_u64(h, static_cast<std::uint64_t>(rule.frequency));
    return h;
}

std::uint64_t hash_trail(const std::vector<SymbolId>& trail) noexcept {
    return hash_trail_range(trail, 0, trail.size());
}

IntegrityManifest build_integrity_manifest(
    const std::vector<AdaptiveRule>& rules,
    const std::vector<SymbolId>& trail,
    std::size_t trail_block_size) {
    IntegrityManifest manifest;
    if (trail_block_size == 0) trail_block_size = 64;
    manifest.trail_block_size = trail_block_size;

    manifest.rule_hashes.reserve(rules.size());
    for (const auto& rule : rules) manifest.rule_hashes.push_back(hash_rule(rule));
    manifest.trail_hash = hash_trail(trail);

    for (std::size_t begin = 0; begin < trail.size(); begin += trail_block_size) {
        const auto end = std::min(begin + trail_block_size, trail.size());
        manifest.trail_block_hashes.push_back(hash_trail_range(trail, begin, end));
    }
    return manifest;
}

std::vector<std::size_t> find_corrupted_rules(
    const IntegrityManifest& manifest,
    const std::vector<AdaptiveRule>& rules) {
    std::vector<std::size_t> bad;
    const auto common = std::min(rules.size(), manifest.rule_hashes.size());
    for (std::size_t i = 0; i < common; ++i) {
        if (hash_rule(rules[i]) != manifest.rule_hashes[i]) bad.push_back(i);
    }
    if (rules.size() != manifest.rule_hashes.size()) {
        const auto max_size = std::max(rules.size(), manifest.rule_hashes.size());
        for (std::size_t i = common; i < max_size; ++i) bad.push_back(i);
    }
    return bad;
}

bool verify_trail(const IntegrityManifest& manifest,
                  const std::vector<SymbolId>& trail) noexcept {
    return hash_trail(trail) == manifest.trail_hash;
}

std::vector<std::size_t> find_corrupted_trail_blocks(
    const IntegrityManifest& manifest,
    const std::vector<SymbolId>& trail) {
    std::vector<std::size_t> bad;
    const auto bs = manifest.trail_block_size == 0 ? std::size_t{64} : manifest.trail_block_size;
    const std::size_t block_count = (trail.size() + bs - 1) / bs;
    const std::size_t common = std::min(block_count, manifest.trail_block_hashes.size());

    for (std::size_t b = 0; b < common; ++b) {
        const auto begin = b * bs;
        const auto end = std::min(begin + bs, trail.size());
        if (hash_trail_range(trail, begin, end) != manifest.trail_block_hashes[b]) bad.push_back(b);
    }
    if (block_count != manifest.trail_block_hashes.size()) {
        const auto max_count = std::max(block_count, manifest.trail_block_hashes.size());
        for (std::size_t b = common; b < max_count; ++b) bad.push_back(b);
    }
    return bad;
}

} // namespace bit_analyze
