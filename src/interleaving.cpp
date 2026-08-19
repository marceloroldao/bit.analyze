#include "bit_analyze/interleaving.hpp"

#include <algorithm>

namespace bit_analyze {

std::vector<std::size_t> interleaved_order(std::size_t symbol_count,
                                           std::size_t lanes) {
    if (lanes == 0) lanes = 1;
    lanes = std::min(lanes, symbol_count == 0 ? std::size_t{1} : symbol_count);

    std::vector<std::size_t> order;
    order.reserve(symbol_count);

    for (std::size_t lane = 0; lane < lanes; ++lane) {
        for (std::size_t i = lane; i < symbol_count; i += lanes) {
            order.push_back(i);
        }
    }
    return order;
}

std::vector<std::size_t> inverse_interleaved_order(std::size_t symbol_count,
                                                   std::size_t lanes) {
    const auto order = interleaved_order(symbol_count, lanes);
    std::vector<std::size_t> inverse(symbol_count);
    for (std::size_t physical = 0; physical < order.size(); ++physical) {
        inverse[order[physical]] = physical;
    }
    return inverse;
}

} // namespace bit_analyze
