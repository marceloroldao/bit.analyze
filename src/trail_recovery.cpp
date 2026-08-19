#include "bit_analyze/trail_recovery.hpp"

#include <algorithm>
#include <stdexcept>

namespace bit_analyze {

namespace {

std::uint8_t gf_mul(std::uint8_t a, std::uint8_t b) noexcept {
    std::uint8_t p = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1U) p ^= a;
        const bool hi = (a & 0x80U) != 0;
        a <<= 1U;
        if (hi) a ^= 0x1dU;
        b >>= 1U;
    }
    return p;
}

std::uint8_t gf_pow(std::uint8_t a, int n) noexcept {
    std::uint8_t r = 1;
    while (n > 0) {
        if (n & 1) r = gf_mul(r, a);
        a = gf_mul(a, a);
        n >>= 1;
    }
    return r;
}

std::uint8_t gf_inv(std::uint8_t a) {
    if (a == 0) throw std::runtime_error("GF(256) inverse of zero");
    return gf_pow(a, 254);
}

std::array<std::uint8_t, 8> to_bytes(SymbolId v) noexcept {
    std::array<std::uint8_t, 8> out{};
    for (std::size_t i = 0; i < 8; ++i) {
        out[i] = static_cast<std::uint8_t>((v >> (8U * i)) & 0xFFU);
    }
    return out;
}

SymbolId from_bytes(const std::array<std::uint8_t, 8>& b) noexcept {
    SymbolId v = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        v |= static_cast<SymbolId>(b[i]) << (8U * i);
    }
    return v;
}

std::vector<SymbolId> block_slice(const std::vector<SymbolId>& trail,
                                  std::size_t begin,
                                  std::size_t count) {
    if (begin > trail.size()) return {};
    const auto end = std::min(begin + count, trail.size());
    return std::vector<SymbolId>(trail.begin() + static_cast<std::ptrdiff_t>(begin),
                                 trail.begin() + static_cast<std::ptrdiff_t>(end));
}

bool block_hash_matches(const IntegrityManifest& manifest,
                        const std::vector<SymbolId>& trail,
                        std::size_t block_index) {
    if (block_index >= manifest.trail_block_hashes.size()) return false;
    const auto bs = manifest.trail_block_size == 0 ? std::size_t{64}
                                                   : manifest.trail_block_size;
    const auto begin = block_index * bs;
    const auto block = block_slice(trail, begin, bs);
    return hash_trail(block) == manifest.trail_block_hashes[block_index];
}

} // namespace

std::vector<TrailParityBlock> build_trail_parity(
    const std::vector<SymbolId>& trail,
    std::size_t block_size) {
    std::vector<TrailParityBlock> out;
    if (block_size == 0) return out;

    for (std::size_t begin = 0; begin < trail.size(); begin += block_size) {
        TrailParityBlock b;
        b.begin_index = begin;
        b.count = std::min(block_size, trail.size() - begin);
        for (std::size_t i = begin; i < begin + b.count; ++i) {
            b.xor_symbol ^= trail[i];
        }
        out.push_back(b);
    }
    return out;
}

std::vector<TrailDualParityBlock> build_trail_dual_parity(
    const std::vector<SymbolId>& trail,
    std::size_t block_size) {
    std::vector<TrailDualParityBlock> out;
    if (block_size == 0 || block_size > 255) return out;

    for (std::size_t begin = 0; begin < trail.size(); begin += block_size) {
        TrailDualParityBlock b;
        b.begin_index = begin;
        b.count = std::min(block_size, trail.size() - begin);

        for (std::size_t local = 0; local < b.count; ++local) {
            const auto bytes = to_bytes(trail[begin + local]);
            const auto coeff = static_cast<std::uint8_t>(local + 1U);
            for (std::size_t k = 0; k < 8; ++k) {
                b.p[k] ^= bytes[k];
                b.q[k] ^= gf_mul(coeff, bytes[k]);
            }
        }
        out.push_back(b);
    }
    return out;
}

std::optional<std::vector<SymbolId>> recover_one_symbol_in_block(
    const IntegrityManifest& manifest,
    const TrailParityBlock& parity,
    const std::vector<SymbolId>& damaged_trail,
    std::size_t block_index) {
    if (parity.begin_index + parity.count > damaged_trail.size()) return std::nullopt;

    std::optional<std::vector<SymbolId>> unique;

    for (std::size_t candidate = 0; candidate < parity.count; ++candidate) {
        SymbolId recovered = parity.xor_symbol;
        for (std::size_t local = 0; local < parity.count; ++local) {
            if (local == candidate) continue;
            recovered ^= damaged_trail[parity.begin_index + local];
        }

        auto repaired = damaged_trail;
        repaired[parity.begin_index + candidate] = recovered;
        if (!block_hash_matches(manifest, repaired, block_index)) continue;

        if (unique.has_value() && *unique != repaired) {
            return std::nullopt;
        }
        unique = std::move(repaired);
    }

    return unique;
}

std::optional<std::vector<SymbolId>> recover_two_symbols_in_block(
    const IntegrityManifest& manifest,
    const TrailDualParityBlock& parity,
    const std::vector<SymbolId>& damaged_trail,
    std::size_t block_index) {
    if (parity.begin_index + parity.count > damaged_trail.size()) return std::nullopt;
    if (parity.count < 2 || parity.count > 255) return std::nullopt;

    std::optional<std::vector<SymbolId>> unique;

    for (std::size_t a = 0; a < parity.count; ++a) {
        for (std::size_t b = a + 1; b < parity.count; ++b) {
            std::array<std::uint8_t, 8> s = parity.p;
            std::array<std::uint8_t, 8> t = parity.q;

            for (std::size_t local = 0; local < parity.count; ++local) {
                if (local == a || local == b) continue;
                const auto bytes = to_bytes(damaged_trail[parity.begin_index + local]);
                const auto coeff = static_cast<std::uint8_t>(local + 1U);
                for (std::size_t k = 0; k < 8; ++k) {
                    s[k] ^= bytes[k];
                    t[k] ^= gf_mul(coeff, bytes[k]);
                }
            }

            const auto ca = static_cast<std::uint8_t>(a + 1U);
            const auto cb = static_cast<std::uint8_t>(b + 1U);
            const auto denom = static_cast<std::uint8_t>(ca ^ cb);
            if (denom == 0) continue;
            const auto inv = gf_inv(denom);

            std::array<std::uint8_t, 8> xa{};
            std::array<std::uint8_t, 8> xb{};
            for (std::size_t k = 0; k < 8; ++k) {
                const auto rhs = static_cast<std::uint8_t>(t[k] ^ gf_mul(cb, s[k]));
                xa[k] = gf_mul(inv, rhs);
                xb[k] = static_cast<std::uint8_t>(s[k] ^ xa[k]);
            }

            auto repaired = damaged_trail;
            repaired[parity.begin_index + a] = from_bytes(xa);
            repaired[parity.begin_index + b] = from_bytes(xb);
            if (!block_hash_matches(manifest, repaired, block_index)) continue;

            if (unique.has_value() && *unique != repaired) {
                return std::nullopt;
            }
            unique = std::move(repaired);
        }
    }

    return unique;
}

} // namespace bit_analyze
