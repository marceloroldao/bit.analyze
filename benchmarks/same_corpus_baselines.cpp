#include "bit_analyze/adaptive_memory.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {
std::vector<std::uint8_t> read_all(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::size_t rle_size(const std::vector<std::uint8_t>& d) {
    if (d.empty()) return 0;
    std::size_t out = 0;
    for (std::size_t i = 0; i < d.size();) {
        std::size_t j = i + 1;
        while (j < d.size() && d[j] == d[i] && j - i < 255) ++j;
        out += 2; // value + run length
        i = j;
    }
    return out;
}

// Small deterministic LZ77-style baseline. Token model: literal=2 bytes,
// match=4 bytes. This is intentionally simple and not a zstd/gzip substitute.
std::size_t lz77_size(const std::vector<std::uint8_t>& d) {
    constexpr std::size_t W = 4096, MIN = 4, MAX = 255;
    std::size_t out = 0;
    for (std::size_t i = 0; i < d.size();) {
        std::size_t best = 0;
        const auto begin = i > W ? i - W : 0;
        for (std::size_t p = begin; p < i; ++p) {
            std::size_t n = 0;
            while (n < MAX && i + n < d.size() && p + n < i && d[p + n] == d[i + n]) ++n;
            best = std::max(best, n);
        }
        if (best >= MIN) { out += 4; i += best; }
        else { out += 2; ++i; }
    }
    return out;
}

struct Fixed8Stats {
    std::size_t unique_payload{};
    std::size_t references{};
};

Fixed8Stats fixed8_global(const std::vector<std::vector<std::uint8_t>>& corpus) {
    std::unordered_set<std::string> unique;
    std::size_t refs = 0;
    for (const auto& d : corpus) {
        for (std::size_t i = 0; i < d.size(); i += 8) {
            const auto n = std::min<std::size_t>(8, d.size() - i);
            unique.emplace(reinterpret_cast<const char*>(d.data() + i), n);
            ++refs;
        }
    }
    std::size_t payload = 0;
    for (const auto& s : unique) payload += s.size();
    return {payload, refs};
}

std::size_t global_bigram_vocab_bytes(const std::vector<std::vector<std::uint8_t>>& corpus) {
    std::array<bool, 65536> seen{};
    std::size_t count = 0;
    for (const auto& d : corpus) {
        for (std::size_t i = 0; i + 1 < d.size(); ++i) {
            const auto k = (static_cast<unsigned>(d[i]) << 8) | d[i + 1];
            if (!seen[k]) { seen[k] = true; ++count; }
        }
    }
    return count * 2;
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: bit_analyze_same_corpus_baselines FILE [FILE ...]\n";
        return 2;
    }

    bit_analyze::AdaptiveMemory memory;
    std::vector<std::vector<std::uint8_t>> corpus;
    std::vector<std::string> names;
    for (int i = 1; i < argc; ++i) {
        auto d = read_all(argv[i]);
        if (!d.empty()) { names.emplace_back(argv[i]); corpus.push_back(std::move(d)); }
    }
    if (corpus.empty()) return 3;

    memory.train(corpus, 256, 3, 1.5, 0.001);

    std::size_t input_total = 0;
    std::size_t adaptive_trail_bytes = 0;
    std::size_t rle_total = 0;
    std::size_t lz77_total = 0;

    std::cout << "file,input,adaptive_trail_bytes,rle_bytes,lz77_bytes\n";
    for (std::size_t i = 0; i < corpus.size(); ++i) {
        const auto& d = corpus[i];
        const auto e = memory.encode(d);
        if (memory.decode(e.trail) != d) return 4;

        const auto trail_bytes = e.trail.size() * sizeof(bit_analyze::SymbolId);
        const auto rb = rle_size(d);
        const auto lb = lz77_size(d);
        std::cout << names[i] << ',' << d.size() << ',' << trail_bytes << ',' << rb << ',' << lb << '\n';

        input_total += d.size();
        adaptive_trail_bytes += trail_bytes;
        rle_total += rb;
        lz77_total += lb;
    }

    // Portable estimate: each adaptive rule serializes id,left,right,frequency as 4x u64.
    const std::size_t adaptive_dictionary_bytes = memory.rule_count() * 32;
    const std::size_t adaptive_total = adaptive_dictionary_bytes + adaptive_trail_bytes;

    // Fixed 8-byte dedup baseline: unique payload + one 64-bit reference per occurrence.
    const auto fixed = fixed8_global(corpus);
    const std::size_t fixed8_total = fixed.unique_payload + fixed.references * 8;

    // Byte-bigram is reported as a structural vocabulary baseline, not compression.
    const auto bigram_vocab_bytes = global_bigram_vocab_bytes(corpus);

    auto ratio = [input_total](std::size_t n) {
        return input_total ? static_cast<double>(n) / static_cast<double>(input_total) : 0.0;
    };

    std::cout << std::fixed << std::setprecision(6)
              << "summary,input_total=" << input_total
              << ",adaptive_rules=" << memory.rule_count()
              << ",adaptive_dictionary_bytes=" << adaptive_dictionary_bytes
              << ",adaptive_trail_bytes=" << adaptive_trail_bytes
              << ",adaptive_total_bytes=" << adaptive_total
              << ",adaptive_total_ratio=" << ratio(adaptive_total)
              << ",fixed8_unique_payload=" << fixed.unique_payload
              << ",fixed8_references=" << fixed.references
              << ",fixed8_total_bytes=" << fixed8_total
              << ",fixed8_total_ratio=" << ratio(fixed8_total)
              << ",bigram_vocab_bytes=" << bigram_vocab_bytes
              << ",rle_total_bytes=" << rle_total
              << ",rle_ratio=" << ratio(rle_total)
              << ",lz77_total_bytes=" << lz77_total
              << ",lz77_ratio=" << ratio(lz77_total)
              << '\n';
    return 0;
}
