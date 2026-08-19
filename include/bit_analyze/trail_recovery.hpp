#pragma once

#include "bit_analyze/hierarchical_memory.hpp"
#include "bit_analyze/integrity.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace bit_analyze {

struct TrailParityBlock {
    std::size_t begin_index{};
    std::size_t count{};
    SymbolId xor_symbol{};
};

struct TrailDualParityBlock {
    std::size_t begin_index{};
    std::size_t count{};
    std::array<std::uint8_t, 8> p{};
    std::array<std::uint8_t, 8> q{};
};

std::vector<TrailParityBlock> build_trail_parity(
    const std::vector<SymbolId>& trail,
    std::size_t block_size = 64);

std::vector<TrailDualParityBlock> build_trail_dual_parity(
    const std::vector<SymbolId>& trail,
    std::size_t block_size = 64);

// The integrity manifest identifies the damaged block. The exact damaged
// symbol position is inferred by trying candidates and accepting only the
// repair that restores the stored block hash.
std::optional<std::vector<SymbolId>> recover_one_symbol_in_block(
    const IntegrityManifest& manifest,
    const TrailParityBlock& parity,
    const std::vector<SymbolId>& damaged_trail,
    std::size_t block_index);

// Same principle for two damaged symbols. Two independent GF(256) parity
// equations recover each candidate pair, and the block hash disambiguates
// the correct pair.
std::optional<std::vector<SymbolId>> recover_two_symbols_in_block(
    const IntegrityManifest& manifest,
    const TrailDualParityBlock& parity,
    const std::vector<SymbolId>& damaged_trail,
    std::size_t block_index);

} // namespace bit_analyze
