#include "bit_analyze/hierarchical_memory.hpp"

#include <stdexcept>

namespace bit_analyze {

HierarchicalMemory::HierarchicalMemory() = default;

std::size_t HierarchicalMemory::PairHash::operator()(const std::pair<SymbolId, SymbolId>& p) const noexcept {
    const auto h1 = std::hash<SymbolId>{}(p.first);
    const auto h2 = std::hash<SymbolId>{}(p.second);
    return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6U) + (h1 >> 2U));
}

SymbolId HierarchicalMemory::get_or_create_relation(SymbolId left, SymbolId right, std::uint32_t layer) {
    const std::pair<SymbolId, SymbolId> key{left, right};
    const auto it = relation_index_.find(key);
    if (it != relation_index_.end()) {
        auto& node = relations_.at(static_cast<std::size_t>(it->second - kBaseSymbolCount));
        ++node.ref_count;
        return it->second;
    }

    const SymbolId id = kBaseSymbolCount + relations_.size();
    relations_.push_back(RelationNode{id, left, right, layer, 1});
    relation_index_.emplace(key, id);
    return id;
}

EncodeResult HierarchicalMemory::encode(const std::vector<std::uint8_t>& data, std::uint32_t max_layers) {
    EncodeResult result;
    result.input_bytes = data.size();
    result.trail.reserve(data.size());

    for (const auto byte : data) {
        result.trail.push_back(static_cast<SymbolId>(byte));
    }

    for (std::uint32_t layer = 1; layer <= max_layers && result.trail.size() >= 2; ++layer) {
        std::vector<SymbolId> next;
        next.reserve((result.trail.size() + 1) / 2);

        std::size_t i = 0;
        for (; i + 1 < result.trail.size(); i += 2) {
            next.push_back(get_or_create_relation(result.trail[i], result.trail[i + 1], layer));
            ++result.relations_used;
        }
        if (i < result.trail.size()) {
            next.push_back(result.trail[i]);
        }

        result.trail = std::move(next);
    }

    return result;
}

void HierarchicalMemory::expand(SymbolId symbol, std::vector<std::uint8_t>& out) const {
    if (symbol < kBaseSymbolCount) {
        out.push_back(static_cast<std::uint8_t>(symbol));
        return;
    }

    const auto index = static_cast<std::size_t>(symbol - kBaseSymbolCount);
    if (index >= relations_.size()) {
        throw std::runtime_error("invalid relation symbol");
    }

    const auto& node = relations_[index];
    expand(node.left, out);
    expand(node.right, out);
}

std::vector<std::uint8_t> HierarchicalMemory::decode(const std::vector<SymbolId>& trail) const {
    std::vector<std::uint8_t> out;
    for (const auto symbol : trail) {
        expand(symbol, out);
    }
    return out;
}

std::size_t HierarchicalMemory::relation_count() const noexcept {
    return relations_.size();
}

std::size_t HierarchicalMemory::symbol_count() const noexcept {
    return kBaseSymbolCount + relations_.size();
}

const std::vector<RelationNode>& HierarchicalMemory::relations() const noexcept {
    return relations_;
}

} // namespace bit_analyze
