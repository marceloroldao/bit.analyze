#pragma once

#include "bit_analyze/adaptive_memory.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace bit_analyze {

struct MemorySnapshot {
    std::uint32_t version{1};
    std::vector<AdaptiveRule> rules;
    std::vector<std::vector<SymbolId>> trails;
};

void save_snapshot(const MemorySnapshot& snapshot, const std::string& path);
MemorySnapshot load_snapshot(const std::string& path);

} // namespace bit_analyze
