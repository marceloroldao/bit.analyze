#include "bit_analyze/adaptive_memory.hpp"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::vector<std::uint8_t> read_window(const std::string& path,
                                      std::size_t skip,
                                      std::size_t bytes) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    in.seekg(static_cast<std::streamoff>(skip));
    std::vector<std::uint8_t> out(bytes);
    in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(bytes));
    out.resize(static_cast<std::size_t>(in.gcount()));
    return out;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: bit_analyze_real_file_benchmark FILE [FILE ...]\n";
        return 2;
    }

    constexpr std::size_t kSkip = 512;
    constexpr std::size_t kWindow = 2048;

    bit_analyze::AdaptiveMemory memory;
    std::vector<std::vector<std::uint8_t>> samples;
    std::vector<std::vector<bit_analyze::SymbolId>> stored_trails;

    std::cout << "file,bytes,new_rules,cost_before_consolidation\n";

    for (int i = 1; i < argc; ++i) {
        auto data = read_window(argv[i], kSkip, kWindow);
        if (data.empty()) continue;

        const auto learned = memory.learn_online(data, 4, 3, 1.5, 0.001);
        const auto encoded = memory.encode(data);
        assert(memory.decode(encoded.trail) == data);

        const double cost = static_cast<double>(encoded.trail.size()) /
                            static_cast<double>(data.size());
        std::cout << argv[i] << ',' << data.size() << ',' << learned << ','
                  << std::fixed << std::setprecision(6) << cost << '\n';

        samples.push_back(std::move(data));
        stored_trails.push_back(encoded.trail);
    }

    std::vector<std::vector<bit_analyze::SymbolId>> current_trails;
    current_trails.reserve(samples.size());
    for (const auto& sample : samples) {
        current_trails.push_back(memory.encode(sample).trail);
    }

    const auto before = memory.rule_count();
    const auto meta = memory.consolidate(current_trails, 32, 3, 1.5, 0.001);
    const auto after = memory.rule_count();

    double mean_cost = 0.0;
    for (const auto& sample : samples) {
        const auto encoded = memory.encode(sample);
        assert(memory.decode(encoded.trail) == sample);
        mean_cost += static_cast<double>(encoded.trail.size()) /
                     static_cast<double>(sample.size());
    }
    if (!samples.empty()) mean_cost /= static_cast<double>(samples.size());

    bool old_trails_ok = true;
    for (std::size_t i = 0; i < stored_trails.size(); ++i) {
        if (memory.decode(stored_trails[i]) != samples[i]) {
            old_trails_ok = false;
            break;
        }
    }
    assert(old_trails_ok);

    std::cout << "summary,rules_before=" << before
              << ",meta_added=" << meta
              << ",rules_after=" << after
              << ",mean_cost_after=" << std::fixed << std::setprecision(6) << mean_cost
              << ",old_trails_ok=" << (old_trails_ok ? 1 : 0) << '\n';

    return 0;
}
