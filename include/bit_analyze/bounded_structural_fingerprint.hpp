#pragma once
#include "bit_analyze/structural_fingerprint.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>
#include <string>
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
struct BoundedStructuralFingerprint8 {
 std::uint32_t version{2};
 std::uint64_t total_bytes{};
 std::size_t event_count{};
 std::size_t renormalizations{};
 BoundedStructuralFingerprintConfig config{};
 std::vector<std::uint8_t> frequency;
 std::vector<std::uint8_t> regional_frequency;
 std::vector<std::uint8_t> transitions;
 std::size_t storage_bytes() const noexcept { return frequency.size()+regional_frequency.size()+transitions.size(); }
};
class BoundedStructuralFingerprintAccumulator8 {
public:
 explicit BoundedStructuralFingerprintAccumulator8(std::uint64_t total_bytes,BoundedStructuralFingerprintConfig config={});
 void observe(const StructuralEvent& event);
 const BoundedStructuralFingerprint8& fingerprint() const noexcept { return fingerprint_; }
private:
 void increment(std::vector<std::uint8_t>& counters,std::size_t index);
 BoundedStructuralFingerprint8 fingerprint_;
 bool have_previous_{false};
 SymbolId previous_{};
};
struct RollingStructuralFingerprintConfig {
 BoundedStructuralFingerprintConfig bounded{};
 std::uint64_t region_span_bytes{65536};
};
struct RollingStructuralFingerprint8 {
 std::uint32_t version{3};
 std::size_t event_count{};
 std::size_t renormalizations{};
 std::size_t region_reuses{};
 std::uint64_t latest_epoch{};
 RollingStructuralFingerprintConfig config{};
 std::vector<std::uint8_t> frequency;
 std::vector<std::uint8_t> regional_frequency;
 std::vector<std::uint8_t> transitions;
 std::vector<std::uint64_t> region_epochs;
 std::size_t storage_bytes() const noexcept { return frequency.size()+regional_frequency.size()+transitions.size()+region_epochs.size()*sizeof(std::uint64_t); }
};
class RollingStructuralFingerprintAccumulator8 {
public:
 explicit RollingStructuralFingerprintAccumulator8(RollingStructuralFingerprintConfig config={});
 void observe(const StructuralEvent& event);
 const RollingStructuralFingerprint8& fingerprint() const noexcept { return fingerprint_; }
private:
 void increment(std::vector<std::uint8_t>& counters,std::size_t index);
 void increment_region(std::size_t region,std::size_t bucket_index);
 RollingStructuralFingerprint8 fingerprint_;
 bool have_previous_{false};
 bool have_event_{false};
 SymbolId previous_{};
 std::uint64_t last_offset_{};
};
class RollingStructuralStream8 {
public:
 RollingStructuralStream8(StructuralStream& stream,RollingStructuralFingerprintAccumulator8& accumulator);
 std::vector<StructuralEvent> push(const std::uint8_t* data,std::size_t size,const std::string& source_id);
 std::vector<StructuralEvent> push(const std::vector<std::uint8_t>& data,const std::string& source_id);
 std::vector<StructuralEvent> flush(const std::string& source_id);
 const RollingStructuralFingerprint8& fingerprint() const noexcept { return accumulator_.fingerprint(); }
private:
 void observe(const std::vector<StructuralEvent>& events);
 StructuralStream& stream_;
 RollingStructuralFingerprintAccumulator8& accumulator_;
};
BoundedStructuralFingerprint make_bounded_structural_fingerprint(const std::vector<StructuralEvent>& events,std::uint64_t total_bytes,BoundedStructuralFingerprintConfig config={});
StructuralSimilarity compare_bounded_structural_fingerprints(const BoundedStructuralFingerprint& a,const BoundedStructuralFingerprint& b);
BoundedStructuralFingerprint expand_bounded_structural_fingerprint(const BoundedStructuralFingerprint8& value);
} // namespace bit_analyze
