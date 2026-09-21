#pragma once

#include "bit_analyze/structural_stream.hpp"
#include "bit_analyze/hierarchical_memory.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string>

namespace bit_analyze {

struct StructuralIngestConfig {
    std::size_t window_size{4096};
    std::size_t hop_size{4096};
    std::size_t chunk_size{64 * 1024};
    std::uint32_t max_layers{2};
};

struct StructuralIngestStats {
    std::uint64_t input_bytes{};
    std::uint64_t events{};
    std::size_t peak_buffered_bytes{};
    std::size_t relation_count{};
};

using StructuralEventSink = std::function<void(const StructuralEvent&)>;

/*
 * Modality-agnostic bounded-memory ingestion.
 *
 * Input is treated only as bytes. No HTML/image/audio/video semantics or
 * provider-specific rules are applied here. Chunk boundaries must not affect
 * the emitted StructuralEvent sequence.
 */
StructuralIngestStats ingest_structural_stream(
    std::istream& input,
    HierarchicalMemory& memory,
    const std::string& source_id,
    const StructuralIngestConfig& config,
    const StructuralEventSink& sink
);

} // namespace bit_analyze
