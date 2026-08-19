#include "bit_analyze/adaptive_memory.hpp"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

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

template <class F>
long long micros(F&& fn) {
    const auto t0 = Clock::now();
    fn();
    const auto t1 = Clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}

void run_size(std::size_t window, int argc, char** argv) {
    constexpr std::size_t kSkip = 512;

    bit_analyze::AdaptiveMemory memory;
    std::vector<std::vector<std::uint8_t>> samples;
    std::vector<std::vector<bit_analyze::SymbolId>> stored_trails;

    long long learn_us = 0;
    long long encode_us = 0;
    long long decode_us = 0;

    for (int i = 1; i < argc; ++i) {
        auto data = read_window(argv[i], kSkip, window);
        if (data.size() < window / 2) continue;

        std::size_t learned = 0;
        learn_us += micros([&] {
            learned = memory.learn_online(data, 8, 4, 1.5, 0.001);
        });
        (void)learned;

        bit_analyze::AdaptiveEncodeResult encoded;
        encode_us += micros([&] { encoded = memory.encode(data); });

        std::vector<std::uint8_t> decoded;
        decode_us += micros([&] { decoded = memory.decode(encoded.trail); });
        assert(decoded == data);

        stored_trails.push_back(encoded.trail);
        samples.push_back(std::move(data));
    }

    if (samples.empty()) return;

    std::vector<std::vector<bit_analyze::SymbolId>> current_trails;
    current_trails.reserve(samples.size());
    for (const auto& sample : samples) {
        current_trails.push_back(memory.encode(sample).trail);
    }

    const auto rules_before = memory.rule_count();
    std::size_t meta_added = 0;
    const auto consolidate_us = micros([&] {
        meta_added = memory.consolidate(current_trails, 64, 4, 1.5, 0.001);
    });

    double mean_cost = 0.0;
    std::size_t total_trail_symbols = 0;
    for (const auto& sample : samples) {
        const auto encoded = memory.encode(sample);
        assert(memory.decode(encoded.trail) == sample);
        total_trail_symbols += encoded.trail.size();
        mean_cost += static_cast<double>(encoded.trail.size()) /
                     static_cast<double>(sample.size());
    }
    mean_cost /= static_cast<double>(samples.size());

    bool old_trails_ok = true;
    for (std::size_t i = 0; i < stored_trails.size(); ++i) {
        if (memory.decode(stored_trails[i]) != samples[i]) {
            old_trails_ok = false;
            break;
        }
    }
    assert(old_trails_ok);

    const std::size_t total_input_bytes = samples.size() * window;
    const double rules_per_kib = static_cast<double>(memory.rule_count()) /
                                 (static_cast<double>(total_input_bytes) / 1024.0);

    std::cout << window << ','
              << samples.size() << ','
              << total_input_bytes << ','
              << rules_before << ','
              << meta_added << ','
              << memory.rule_count() << ','
              << total_trail_symbols << ','
              << std::fixed << std::setprecision(6)
              << mean_cost << ','
              << rules_per_kib << ','
              << learn_us << ','
              << consolidate_us << ','
              << encode_us << ','
              << decode_us << ','
              << (old_trails_ok ? 1 : 0)
              << '\n';
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: bit_analyze_real_scale_benchmark FILE [FILE ...]\n";
        return 2;
    }

    std::cout << "window_bytes,files,total_input_bytes,rules_before,meta_added,rules_after,total_trail_symbols,mean_cost_per_byte,rules_per_kib,learn_us,consolidate_us,encode_us,decode_us,old_trails_ok\n";

    const std::vector<std::size_t> windows{
        2 * 1024,
        8 * 1024,
        32 * 1024,
        128 * 1024,
        512 * 1024
    };

    for (const auto window : windows) {
        run_size(window, argc, argv);
    }

    return 0;
}
