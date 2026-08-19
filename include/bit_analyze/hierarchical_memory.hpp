#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bit_analyze {

using SymbolId = std::uint64_t;

struct RelationNode {
    SymbolId id{};
    SymbolId left{};
    SymbolId right{};
    std::uint32_t layer{};
    std::uint64_t ref_count{};
};

struct EncodeResult {
    std::vector<SymbolId> trail;
    std::size_t input_bytes{};
    std::size_t relations_used{};
};

class HierarchicalMemory {
public:
    HierarchicalMemory();

    EncodeResult encode(const std::vector<std::uint8_t>& data, std::uint32_t max_layers = 1);
    std::vector<std::uint8_t> decode(const std::vector<SymbolId>& trail) const;

    std::size_t relation_count() const noexcept;
    std::size_t symbol_count() const noexcept;
    const std::vector<RelationNode>& relations() const noexcept;

private:
    struct PairHash {
        std::size_t operator()(const std::pair<SymbolId, SymbolId>& p) const noexcept;
    };

    SymbolId get_or_create_relation(SymbolId left, SymbolId right, std::uint32_t layer);
    void expand(SymbolId symbol, std::vector<std::uint8_t>& out) const;

    static constexpr SymbolId kBaseSymbolCount = 256;

    std::unordered_map<std::pair<SymbolId, SymbolId>, SymbolId, PairHash> relation_index_;
    std::vector<RelationNode> relations_;
};

} // namespace bit_analyze
