#pragma once

#include "bit_analyze/hierarchical_memory.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace bit_analyze {

struct AdaptiveRule {
    SymbolId id{};
    SymbolId left{};
    SymbolId right{};
    std::size_t frequency{};
};

struct AdaptiveEncodeResult {
    std::vector<SymbolId> trail;
    std::size_t input_bytes{};
    std::size_t rules_applied{};
};

class AdaptiveMemory {
public:
    AdaptiveMemory();

    void train(const std::vector<std::vector<std::uint8_t>>& corpus,
               std::size_t max_rules = 256,
               std::size_t min_frequency = 2,
               double min_lift = 1.0,
               double min_support = 0.0);

    // Append-only online learning. Existing rules and IDs are never changed,
    // so trails encoded before this call remain decodable afterwards.
    std::size_t learn_online(const std::vector<std::uint8_t>& data,
                             std::size_t max_new_rules = 32,
                             std::size_t min_frequency = 2,
                             double min_lift = 1.0,
                             double min_support = 0.0);

    AdaptiveEncodeResult encode(const std::vector<std::uint8_t>& data) const;
    std::vector<std::uint8_t> decode(const std::vector<SymbolId>& trail) const;

    std::size_t rule_count() const noexcept;
    const std::vector<AdaptiveRule>& rules() const noexcept;

private:
    static constexpr SymbolId kBaseSymbolCount = 256;

    static std::size_t pair_frequency(const std::vector<std::vector<SymbolId>>& corpus,
                                      SymbolId left,
                                      SymbolId right);
    static std::vector<SymbolId> replace_pair(const std::vector<SymbolId>& input,
                                              SymbolId left,
                                              SymbolId right,
                                              SymbolId replacement,
                                              std::size_t* replacements = nullptr);
    void expand(SymbolId symbol, std::vector<std::uint8_t>& out) const;

    std::vector<AdaptiveRule> rules_;
};

} // namespace bit_analyze
