#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/hierarchical_memory.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

using Clock = std::chrono::steady_clock;

namespace {

std::vector<std::uint8_t> make_patterned(std::size_t n, std::size_t phase = 0, std::size_t noise_period = 0) {
    static const std::vector<std::uint8_t> motif{
        'A','B','C','D','A','B','C','D','0','0','0','0','X','Y','X','Y'
    };

    std::vector<std::uint8_t> out;
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        auto b = motif[(i + phase) % motif.size()];
        if (noise_period > 0 && i > 0 && i % noise_period == 0) {
            b = static_cast<std::uint8_t>((b + i) & 0xFFU);
        }
        out.push_back(b);
    }
    return out;
}

std::vector<std::uint8_t> make_random(std::size_t n, std::uint32_t seed) {
    std::vector<std::uint8_t> out(n);
    std::uint32_t x = seed;
    for (std::size_t i = 0; i < n; ++i) {
        x ^= x << 13U;
        x ^= x >> 17U;
        x ^= x << 5U;
        out[i] = static_cast<std::uint8_t>(x & 0xFFU);
    }
    return out;
}

template <class F>
long long micros(F&& fn) {
    const auto t0 = Clock::now();
    fn();
    const auto t1 = Clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}

void run_case(const char* label, std::size_t bytes, bool random_case) {
    std::vector<std::vector<std::uint8_t>> train;
    train.reserve(3);

    std::vector<std::uint8_t> probe;
    if (random_case) {
        train.push_back(make_random(bytes, 0x11111111U));
        train.push_back(make_random(bytes, 0x22222222U));
        train.push_back(make_random(bytes, 0x33333333U));
        probe = make_random(bytes, 0x44444444U);
    } else {
        train.push_back(make_patterned(bytes, 0, 0));
        train.push_back(make_patterned(bytes, 1, 257));
        train.push_back(make_patterned(bytes, 3, 509));
        probe = make_patterned(bytes, 5, 389);
    }

    bit_analyze::AdaptiveMemory adaptive;
    long long adaptive_train_us = 0;
    adaptive_train_us = micros([&] {
        // Require both association above chance (lift) and corpus support.
        adaptive.train(train, 128, 4, 1.2, 0.001);
    });

    bit_analyze::AdaptiveEncodeResult adaptive_encoded;
    const auto adaptive_encode_us = micros([&] {
        adaptive_encoded = adaptive.encode(probe);
    });

    std::vector<std::uint8_t> adaptive_decoded;
    const auto adaptive_decode_us = micros([&] {
        adaptive_decoded = adaptive.decode(adaptive_encoded.trail);
    });
    assert(adaptive_decoded == probe);

    bit_analyze::HierarchicalMemory fixed;
    for (const auto& sample : train) {
        const auto e = fixed.encode(sample, 8);
        assert(fixed.decode(e.trail) == sample);
    }

    const auto fixed_before = fixed.relation_count();
    bit_analyze::EncodeResult fixed_encoded;
    const auto fixed_encode_us = micros([&] {
        fixed_encoded = fixed.encode(probe, 8);
    });
    const auto fixed_after = fixed.relation_count();

    std::vector<std::uint8_t> fixed_decoded;
    const auto fixed_decode_us = micros([&] {
        fixed_decoded = fixed.decode(fixed_encoded.trail);
    });
    assert(fixed_decoded == probe);

    const auto fixed_new = fixed_after - fixed_before;

    const double adaptive_inference_cost = static_cast<double>(adaptive_encoded.trail.size());
    const double fixed_inference_cost = static_cast<double>(fixed_encoded.trail.size() + 2 * fixed_new);
    const double adaptive_total_model_cost = static_cast<double>(adaptive_encoded.trail.size() + 2 * adaptive.rule_count());
    const double fixed_total_model_cost = static_cast<double>(fixed_encoded.trail.size() + 2 * fixed_after);

    std::cout << label << ','
              << bytes << ','
              << adaptive.rule_count() << ','
              << adaptive_encoded.trail.size() << ','
              << fixed_before << ','
              << fixed_new << ','
              << fixed_encoded.trail.size() << ','
              << std::fixed << std::setprecision(6)
              << adaptive_inference_cost / static_cast<double>(bytes) << ','
              << fixed_inference_cost / static_cast<double>(bytes) << ','
              << adaptive_total_model_cost / static_cast<double>(bytes) << ','
              << fixed_total_model_cost / static_cast<double>(bytes) << ','
              << adaptive_train_us << ','
              << adaptive_encode_us << ','
              << adaptive_decode_us << ','
              << fixed_encode_us << ','
              << fixed_decode_us
              << '\n';
}

} // namespace

int main() {
    std::cout << "case,bytes,adaptive_rules,adaptive_trail,fixed_relations_before,fixed_new_relations,fixed_trail,adaptive_inference_cost_per_byte,fixed_inference_cost_per_byte,adaptive_total_model_cost_per_byte,fixed_total_model_cost_per_byte,adaptive_train_us,adaptive_encode_us,adaptive_decode_us,fixed_encode_us,fixed_decode_us\n";

    const std::vector<std::size_t> sizes{
        1024,
        10 * 1024,
        100 * 1024,
        1024 * 1024
    };

    for (const auto n : sizes) {
        run_case("patterned", n, false);
    }
    for (const auto n : sizes) {
        run_case("random", n, true);
    }

    return 0;
}
