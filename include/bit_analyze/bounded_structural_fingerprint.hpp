#pragma once
#include "bit_analyze/structural_fingerprint.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
namespace bit_analyze {
struct BoundedStructuralFingerprintConfig {
 std::size_t frequency_buckets{256};
 std::size_t transition_buckets{256};
 std::size_t regions{8};
 std::uint64_t hash_seed{0xB17A2026ULL};
};
struct BoundedStructuralFingerprint {
 std::uint32_t version{1};
 std::uint64_t total_bytes{};
 std::size_t event_count{};
 BoundedStructuralFingerprintConfig config{};
 std::vector<std::uint64_t> frequency;
 std::vector<std::uint64_t> regional_frequency;
 std::vector<std::uint64_t> transitions;
 std::size_t storage_bytes() const noexcept;
};
class BoundedStructuralFingerprintAccumulator {
public:
 explicit BoundedStructuralFingerprintAccumulator(std::uint64_t total_bytes,BoundedStructuralFingerprintConfig config={});
 void observe(const StructuralEvent& event);
 const BoundedStructuralFingerprint& fingerprint() const noexcept { return fingerprint_; }
private:
 BoundedStructuralFingerprint fingerprint_;
 bool have_previous_{false};
 SymbolId previous_{};
};
BoundedStructuralFingerprint make_bounded_structural_fingerprint(const std::vector<StructuralEvent>& events,std::uint64_t total_bytes,BoundedStructuralFingerprintConfig config={});
StructuralSimilarity compare_bounded_structural_fingerprints(const BoundedStructuralFingerprint& a,const BoundedStructuralFingerprint& b);
} // namespace bit_analyze
