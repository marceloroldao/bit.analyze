#pragma once

#include <cstddef>
#include <vector>

namespace bit_analyze {

std::vector<std::size_t> interleaved_order(std::size_t symbol_count,
                                           std::size_t lanes = 64);

std::vector<std::size_t> inverse_interleaved_order(std::size_t symbol_count,
                                                   std::size_t lanes = 64);

} // namespace bit_analyze
