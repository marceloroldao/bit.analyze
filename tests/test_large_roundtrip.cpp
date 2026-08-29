#include "bit_analyze/adaptive_memory.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

std::vector<std::uint8_t> make_data(std::size_t bytes) {
    const std::vector<std::uint8_t> motif{
        'A','B','A','B','C','D','C','D','0','0','0','0','X','Y','X','Y'
    };
    std::vector<std::uint8_t> data;
    data.reserve(bytes);
    for (std::size_t i = 0; i < bytes; ++i) data.push_back(motif[i % motif.size()]);
    return data;
}

void check_size(std::size_t bytes) {
    using namespace bit_analyze;
    AdaptiveMemory memory;
    memory.load_rules({
        AdaptiveRule{256, static_cast<SymbolId>('A'), static_cast<SymbolId>('B'), 1},
        AdaptiveRule{257, static_cast<SymbolId>('C'), static_cast<SymbolId>('D'), 1},
        AdaptiveRule{258, static_cast<SymbolId>('0'), static_cast<SymbolId>('0'), 1},
        AdaptiveRule{259, static_cast<SymbolId>('X'), static_cast<SymbolId>('Y'), 1}
    });

    const auto data = make_data(bytes);
    const auto encoded = memory.encode(data);
    assert(memory.decode(encoded.trail) == data);
}

} // namespace

int main() {
    check_size(1ULL * 1024ULL * 1024ULL);
    check_size(10ULL * 1024ULL * 1024ULL);
    return 0;
}
