#pragma once
#include "bit_analyze/structural_stream.hpp"
#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>
namespace bit_analyze {
struct StructuralFingerprint {
 std::uint32_t version{0};
 std::uint64_t total_bytes{};
 std::size_t event_count{};
 std::map<SymbolId,std::size_t> relation_frequency;
 std::vector<std::map<SymbolId,std::size_t>> regional_frequency;
 std::map<std::pair<SymbolId,SymbolId>,std::size_t> transitions;
};
struct StructuralSimilarity {
 double frequency{};
 double regional{};
 double transitions{};
 double combined{};
};
StructuralFingerprint make_structural_fingerprint(const std::vector<StructuralEvent>& events,std::uint64_t total_bytes,std::size_t regions=8);
StructuralSimilarity compare_structural_fingerprints(const StructuralFingerprint& a,const StructuralFingerprint& b);
} // namespace bit_analyze
