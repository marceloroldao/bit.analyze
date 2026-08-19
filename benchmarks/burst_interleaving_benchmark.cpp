#include <algorithm>
#include <cstddef>
#include <iostream>
#include <unordered_map>

namespace {

bool tolerates_burst(std::size_t total_symbols,
                     std::size_t group_size,
                     std::size_t max_errors_per_group,
                     std::size_t burst_len,
                     bool interleaved) {
    if (group_size == 0 || burst_len == 0 || burst_len > total_symbols) return false;

    const std::size_t step = std::max<std::size_t>(1, total_symbols / 256);
    for (std::size_t start = 0; start + burst_len <= total_symbols; start += step) {
        std::unordered_map<std::size_t, std::size_t> counts;
        for (std::size_t p = start; p < start + burst_len; ++p) {
            const std::size_t group = interleaved ? (p % group_size) : (p / group_size);
            const auto n = ++counts[group];
            if (n > max_errors_per_group) return false;
        }
    }
    return true;
}

std::size_t max_burst(std::size_t total_symbols,
                      std::size_t group_size,
                      std::size_t max_errors_per_group,
                      bool interleaved) {
    std::size_t best = 0;
    for (std::size_t len = 1; len <= total_symbols; ++len) {
        if (!tolerates_burst(total_symbols, group_size, max_errors_per_group, len, interleaved)) break;
        best = len;
    }
    return best;
}

} // namespace

int main() {
    constexpr std::size_t total_symbols = 4096;
    constexpr std::size_t group_size = 64;
    constexpr std::size_t correction_capacity = 2;

    const auto contiguous = max_burst(total_symbols, group_size, correction_capacity, false);
    const auto interleaved = max_burst(total_symbols, group_size, correction_capacity, true);

    std::cout << "total_symbols," << total_symbols << '\n';
    std::cout << "group_size," << group_size << '\n';
    std::cout << "max_errors_per_group," << correction_capacity << '\n';
    std::cout << "contiguous_max_burst," << contiguous << '\n';
    std::cout << "interleaved_max_burst," << interleaved << '\n';
    std::cout << "burst_gain," << (contiguous ? static_cast<double>(interleaved) / contiguous : 0.0) << '\n';

    return 0;
}
