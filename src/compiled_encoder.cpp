#include "bit_analyze/compiled_encoder.hpp"

namespace bit_analyze {

CompiledEncoder::TrieNode::TrieNode() {
    next.fill(-1);
}

CompiledEncoder::CompiledEncoder(const AdaptiveMemory& memory) {
    trie_.emplace_back();

    for (const auto& rule : memory.rules()) {
        const auto bytes = memory.decode(std::vector<SymbolId>{rule.id});
        if (bytes.size() < 2) {
            continue;
        }
        insert_rule(bytes, rule.id);
    }
}

void CompiledEncoder::insert_rule(const std::vector<std::uint8_t>& bytes, SymbolId id) {
    int node = 0;
    for (const auto byte : bytes) {
        int next = trie_[static_cast<std::size_t>(node)].next[byte];
        if (next < 0) {
            next = static_cast<int>(trie_.size());
            trie_[static_cast<std::size_t>(node)].next[byte] = next;
            trie_.emplace_back();
        }
        node = next;
    }

    auto& terminal = trie_[static_cast<std::size_t>(node)];
    if (bytes.size() >= terminal.terminal_length) {
        terminal.terminal = id;
        terminal.terminal_length = bytes.size();
    }
}

CompiledEncodeResult CompiledEncoder::encode(const std::vector<std::uint8_t>& data) const {
    CompiledEncodeResult result;
    result.input_bytes = data.size();
    result.trail.reserve(data.size());

    std::size_t i = 0;
    while (i < data.size()) {
        int node = 0;
        SymbolId best_symbol = 0;
        std::size_t best_length = 0;

        std::size_t j = i;
        while (j < data.size()) {
            const int next = trie_[static_cast<std::size_t>(node)].next[data[j]];
            if (next < 0) {
                break;
            }
            node = next;
            ++j;

            const auto& current = trie_[static_cast<std::size_t>(node)];
            if (current.terminal_length > best_length) {
                best_length = current.terminal_length;
                best_symbol = current.terminal;
            }
        }

        if (best_length >= 2) {
            result.trail.push_back(best_symbol);
            result.matched_bytes += best_length;
            ++result.matched_rules;
            i += best_length;
        } else {
            result.trail.push_back(static_cast<SymbolId>(data[i]));
            ++i;
        }
    }

    return result;
}

std::size_t CompiledEncoder::trie_node_count() const noexcept {
    return trie_.size();
}

} // namespace bit_analyze
