#include "bit_analyze/integrity.hpp"

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
    std::uint64_t h = kOffset;
    mix_u64(h, static_cast<std::uint64_t>(trail.size()));
    for (const auto symbol : trail) {
        mix_u64(h, symbol);
    }
    return h;
}

IntegrityManifest build_integrity_manifest(
    const std::vector<AdaptiveRule>& rules,
    const std::vector<SymbolId>& trail) {
    IntegrityManifest manifest;
    manifest.rule_hashes.reserve(rules.size());
    for (const auto& rule : rules) {
        manifest.rule_hashes.push_back(hash_rule(rule));
    }
    manifest.trail_hash = hash_trail(trail);
    return manifest;
}

std::vector<std::size_t> find_corrupted_rules(
    const IntegrityManifest& manifest,
    const std::vector<AdaptiveRule>& rules) {
    std::vector<std::size_t> bad;
    const auto common = rules.size() < manifest.rule_hashes.size()
        ? rules.size()
        : manifest.rule_hashes.size();

    for (std::size_t i = 0; i < common; ++i) {
        if (hash_rule(rules[i]) != manifest.rule_hashes[i]) {
            bad.push_back(i);
        }
    }

    if (rules.size() != manifest.rule_hashes.size()) {
        const auto max_size = rules.size() > manifest.rule_hashes.size()
            ? rules.size()
            : manifest.rule_hashes.size();
        for (std::size_t i = common; i < max_size; ++i) {
            bad.push_back(i);
        }
    }

    return bad;
}

bool verify_trail(
    const IntegrityManifest& manifest,
    const std::vector<SymbolId>& trail) noexcept {
    return hash_trail(trail) == manifest.trail_hash;
}

} // namespace bit_analyze
