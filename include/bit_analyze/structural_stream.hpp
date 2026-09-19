#pragma once

#include "bit_analyze/hierarchical_memory.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bit_analyze {

struct StructuralEvent {
    std::uint32_t version{1};
    std::string source_id;
    std::uint64_t sequence{};
    std::uint64_t byte_offset{};
    std::uint64_t byte_length{};
    std::vector<SymbolId> trail;
    std::vector<SymbolId> relation_ids;
    std::string signature;
    std::uint32_t resolution{1};

    std::string to_json() const;
};

class StructuralExtractor {
public:
    explicit StructuralExtractor(HierarchicalMemory& memory, std::uint32_t max_layers = 1);
    StructuralEvent extract(const std::vector<std::uint8_t>& bytes,
                            const std::string& source_id,
                            std::uint64_t sequence,
                            std::uint64_t byte_offset);
private:
    HierarchicalMemory& memory_;
    std::uint32_t max_layers_;
};

class StructuralStream {
public:
    StructuralStream(StructuralExtractor& extractor, std::size_t window_size, std::size_t hop_size);

    std::vector<StructuralEvent> push(const std::uint8_t* data, std::size_t size,
                                      const std::string& source_id);
    std::vector<StructuralEvent> push(const std::vector<std::uint8_t>& data,
                                      const std::string& source_id);
    std::vector<StructuralEvent> flush(const std::string& source_id);

    std::size_t buffered_bytes() const noexcept;
    std::uint64_t consumed_bytes() const noexcept;

private:
    std::vector<StructuralEvent> emit_ready(const std::string& source_id);

    StructuralExtractor& extractor_;
    std::size_t window_size_;
    std::size_t hop_size_;
    std::vector<std::uint8_t> buffer_;
    std::uint64_t buffer_offset_{};
    std::uint64_t consumed_bytes_{};
    std::uint64_t sequence_{};
};

} // namespace bit_analyze
