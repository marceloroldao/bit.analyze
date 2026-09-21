#pragma once

#include "bit_analyze/hierarchical_memory.hpp"

#include <string>

namespace bit_analyze {

// Persistence contract for the relation table used by StructuralStream.
// It preserves relation IDs across process restarts and validates a whole-file
// checksum before restoring any state.
void save_hierarchical_state(const HierarchicalMemory& memory, const std::string& path);
void load_hierarchical_state(const std::string& path, HierarchicalMemory& memory);

} // namespace bit_analyze
