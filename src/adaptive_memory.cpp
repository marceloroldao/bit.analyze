#include "bit_analyze/adaptive_memory.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace bit_analyze {

namespace {

struct PairHash {
    std::size_t operator()(const std::pair<SymbolId, SymbolId>& p) const noexcept {
        const auto h1 = std::hash<SymbolId>{}(p.first);
        const auto h2 = std::hash<SymbolId>{}(p.second);
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6U) + (h1 >> 2U));
    }
};

} // namespace

AdaptiveMemory::AdaptiveMemory() = default;

std::vector<SymbolId> AdaptiveMemory::replace_pair(const std::vector<SymbolId>& input,
                                                   SymbolId left,
                                                   SymbolId right,
                                                   SymbolId replacement,
                                                   std::size_t* replacements) {
    std::vector<SymbolId> out;
    out.reserve(input.size());
    std::size_t count = 0;

    std::size_t i = 0;
    while (i < input.size()) {
        if (i + 1 < input.size() && input[i] == left && input[i + 1] == right) {
            out.push_back(replacement);
            i += 2;
            ++count;
        } else {
            out.push_back(input[i]);
            ++i;
        }
    }

    if (replacements) {
        *replacements = count;
    }
    return out;
}

std::size_t AdaptiveMemory::pair_frequency(const std::vector<std::vector<SymbolId>>& corpus,
                                           SymbolId left,
                                           SymbolId right) {
    std::size_t total = 0;
    for (const auto& seq : corpus) {
        for (std::size_t i = 0; i + 1 < seq.size(); ++i) {
            if (seq[i] == left && seq[i + 1] == right) {
                ++total;
            }
        }
    }
    return total;
}

void AdaptiveMemory::train(const std::vector<std::vector<std::uint8_t>>& corpus,
                           std::size_t max_rules,
                           std::size_t min_frequency,
                           double min_lift,
                           double min_support) {
    rules_.clear();

    std::vector<std::vector<SymbolId>> working;
    working.reserve(corpus.size());
    for (const auto& item : corpus) {
        std::vector<SymbolId> seq;
        seq.reserve(item.size());
        for (const auto byte : item) {
            seq.push_back(static_cast<SymbolId>(byte));
        }
        working.push_back(std::move(seq));
    }

    for (std::size_t rule_index = 0; rule_index < max_rules; ++rule_index) {
        std::unordered_map<std::pair<SymbolId, SymbolId>, std::size_t, PairHash> counts;
        std::unordered_map<SymbolId, std::size_t> left_counts;
        std::unordered_map<SymbolId, std::size_t> right_counts;
        std::size_t total_pairs = 0;

        for (const auto& seq : working) {
            for (std::size_t i = 0; i + 1 < seq.size(); ++i) {
                ++counts[{seq[i], seq[i + 1]}];
                ++left_counts[seq[i]];
                ++right_counts[seq[i + 1]];
                ++total_pairs;
            }
        }

        if (counts.empty() || total_pairs == 0) {
            break;
        }

        auto best = counts.end();
        double best_score = 0.0;

        for (auto it = counts.begin(); it != counts.end(); ++it) {
            const auto observed = it->second;
            if (observed < min_frequency) {
                continue;
            }

            const double support = static_cast<double>(observed) /
                                   static_cast<double>(total_pairs);
            if (support < min_support) {
                continue;
            }

            const double expected =
                static_cast<double>(left_counts[it->first.first]) *
                static_cast<double>(right_counts[it->first.second]) /
                static_cast<double>(total_pairs);
            const double lift = expected > 0.0
                ? static_cast<double>(observed) / expected
                : 0.0;
            if (lift < min_lift) {
                continue;
            }

            const double score = static_cast<double>(observed) * lift;
            if (best == counts.end() || score > best_score ||
                (score == best_score && it->first < best->first)) {
                best = it;
                best_score = score;
            }
        }

        if (best == counts.end()) {
            break;
        }

        const SymbolId id = kBaseSymbolCount + rules_.size();
        const auto [left, right] = best->first;
        const auto frequency = best->second;
        rules_.push_back(AdaptiveRule{id, left, right, frequency});

        for (auto& seq : working) {
            seq = replace_pair(seq, left, right, id);
        }
    }
}

std::size_t AdaptiveMemory::learn_online(const std::vector<std::uint8_t>& data,
                                         std::size_t max_new_rules,
                                         std::size_t min_frequency,
                                         double min_lift,
                                         double min_support) {
    // First express the new sample through the vocabulary already known.
    auto working = encode(data).trail;
    std::size_t learned = 0;

    for (std::size_t rule_index = 0; rule_index < max_new_rules; ++rule_index) {
        if (working.size() < 2) {
            break;
        }

        std::unordered_map<std::pair<SymbolId, SymbolId>, std::size_t, PairHash> counts;
        std::unordered_map<SymbolId, std::size_t> left_counts;
        std::unordered_map<SymbolId, std::size_t> right_counts;
        std::size_t total_pairs = 0;

        for (std::size_t i = 0; i + 1 < working.size(); ++i) {
            ++counts[{working[i], working[i + 1]}];
            ++left_counts[working[i]];
            ++right_counts[working[i + 1]];
            ++total_pairs;
        }

        if (counts.empty() || total_pairs == 0) {
            break;
        }

        auto best = counts.end();
        double best_score = 0.0;

        for (auto it = counts.begin(); it != counts.end(); ++it) {
            const auto observed = it->second;
            if (observed < min_frequency) {
                continue;
            }

            const double support = static_cast<double>(observed) /
                                   static_cast<double>(total_pairs);
            if (support < min_support) {
                continue;
            }

            const double expected =
                static_cast<double>(left_counts[it->first.first]) *
                static_cast<double>(right_counts[it->first.second]) /
                static_cast<double>(total_pairs);
            const double lift = expected > 0.0
                ? static_cast<double>(observed) / expected
                : 0.0;
            if (lift < min_lift) {
                continue;
            }

            const double score = static_cast<double>(observed) * lift;
            if (best == counts.end() || score > best_score ||
                (score == best_score && it->first < best->first)) {
                best = it;
                best_score = score;
            }
        }

        if (best == counts.end()) {
            break;
        }

        const SymbolId id = kBaseSymbolCount + rules_.size();
        const auto [left, right] = best->first;
        const auto frequency = best->second;
        rules_.push_back(AdaptiveRule{id, left, right, frequency});
        working = replace_pair(working, left, right, id);
        ++learned;
    }

    return learned;
}

AdaptiveEncodeResult AdaptiveMemory::encode(const std::vector<std::uint8_t>& data) const {
    AdaptiveEncodeResult result;
    result.input_bytes = data.size();
    result.trail.reserve(data.size());

    for (const auto byte : data) {
        result.trail.push_back(static_cast<SymbolId>(byte));
    }

    for (const auto& rule : rules_) {
        std::size_t replacements = 0;
        result.trail = replace_pair(result.trail, rule.left, rule.right, rule.id, &replacements);
        result.rules_applied += replacements;
    }

    return result;
}

void AdaptiveMemory::expand(SymbolId symbol, std::vector<std::uint8_t>& out) const {
    if (symbol < kBaseSymbolCount) {
        out.push_back(static_cast<std::uint8_t>(symbol));
        return;
    }

    const auto index = static_cast<std::size_t>(symbol - kBaseSymbolCount);
    if (index >= rules_.size()) {
        throw std::runtime_error("invalid adaptive relation symbol");
    }

    const auto& rule = rules_[index];
    expand(rule.left, out);
    expand(rule.right, out);
}

std::vector<std::uint8_t> AdaptiveMemory::decode(const std::vector<SymbolId>& trail) const {
    std::vector<std::uint8_t> out;
    for (const auto symbol : trail) {
        expand(symbol, out);
    }
    return out;
}

std::size_t AdaptiveMemory::rule_count() const noexcept {
    return rules_.size();
}

const std::vector<AdaptiveRule>& AdaptiveMemory::rules() const noexcept {
    return rules_;
}

} // namespace bit_analyze
