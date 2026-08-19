#pragma once

#include "bit_analyze/adaptive_memory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace bit_analyze {

struct CompiledEncodeResult {
    std::vector<SymbolId> trail;
    std::size_t input_bytes{};
    std::size_t matched_bytes{};
    std::size_t matched_rules{};
};

class CompiledEncoder {
public:
    explicit CompiledEncoder(const AdaptiveMemory& memory);

    CompiledEncodeResult encode(const std::vector<std::uint8_t>& data) const;
    std::size_t trie_node_count() const noexcept;

private:
    struct TrieNode {
        std::array<int, 256> next{};
        SymbolId terminal{};
        std::size_t terminal_length{};

        TrieNode();
    };

    void insert_rule(const std::vector<std::uint8_t>& bytes, SymbolId id);

    std::vector<TrieNode> trie_;
};

} // namespace bit_analyze
