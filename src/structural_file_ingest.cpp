#include "bit_analyze/structural_file_ingest.hpp"

#include <algorithm>
#include <istream>
#include <stdexcept>
#include <vector>

namespace bit_analyze {

StructuralIngestStats ingest_structural_stream(
    std::istream& input,
    HierarchicalMemory& memory,
    const std::string& source_id,
    const StructuralIngestConfig& config,
    const StructuralEventSink& sink
) {
    if (!input.good() && !input.eof()) throw std::invalid_argument("input stream is not readable");
    if (source_id.empty()) throw std::invalid_argument("source_id must not be empty");
    if (config.window_size == 0 || config.hop_size == 0 || config.hop_size > config.window_size)
        throw std::invalid_argument("require 0 < hop_size <= window_size");
    if (config.chunk_size == 0) throw std::invalid_argument("chunk_size must be > 0");
    if (config.max_layers == 0) throw std::invalid_argument("max_layers must be > 0");
    if (!sink) throw std::invalid_argument("event sink must be set");

    StructuralExtractor extractor(memory, config.max_layers);
    StructuralStream stream(extractor, config.window_size, config.hop_size);
    StructuralIngestStats stats;
    std::vector<std::uint8_t> chunk(config.chunk_size);

    while (input) {
        input.read(reinterpret_cast<char*>(chunk.data()), static_cast<std::streamsize>(chunk.size()));
        const auto count = input.gcount();
        if (count < 0) throw std::runtime_error("negative stream read count");
        if (count == 0) break;

        const auto size = static_cast<std::size_t>(count);
        stats.input_bytes += size;
        auto events = stream.push(chunk.data(), size, source_id);
        stats.peak_buffered_bytes = std::max(stats.peak_buffered_bytes, stream.buffered_bytes());
        for (const auto& event : events) {
            sink(event);
            ++stats.events;
        }
    }
    if (input.bad()) throw std::runtime_error("input stream read failed");

    auto tail = stream.flush(source_id);
    for (const auto& event : tail) {
        sink(event);
        ++stats.events;
    }
    stats.peak_buffered_bytes = std::max(stats.peak_buffered_bytes, stream.buffered_bytes());
    stats.relation_count = memory.relation_count();
    return stats;
}

} // namespace bit_analyze
