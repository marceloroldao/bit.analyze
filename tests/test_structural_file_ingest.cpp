#include "bit_analyze/structural_file_ingest.hpp"

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

using namespace bit_analyze;

struct Run {
    StructuralIngestStats stats;
    std::vector<std::string> events;
};

static Run ingest(const std::string& bytes, std::size_t chunk_size, std::size_t window, std::size_t hop) {
    std::istringstream input(bytes, std::ios::binary);
    HierarchicalMemory memory;
    StructuralIngestConfig config;
    config.chunk_size = chunk_size;
    config.window_size = window;
    config.hop_size = hop;
    config.max_layers = 2;
    Run run;
    run.stats = ingest_structural_stream(
        input,
        memory,
        "raw-web:sha256:test",
        config,
        [&](const StructuralEvent& event) { run.events.push_back(event.to_json()); }
    );
    return run;
}

int main() {
    std::string bytes;
    bytes.reserve(20000);
    for (std::size_t i = 0; i < 20000; ++i)
        bytes.push_back(static_cast<char>((i * 37u + (i / 19u) * 11u) & 0xffu));

    const auto one = ingest(bytes, bytes.size(), 1024, 256);
    const auto bytewise = ingest(bytes, 1, 1024, 256);
    const auto irregular = ingest(bytes, 257, 1024, 256);

    assert(one.events == bytewise.events);
    assert(one.events == irregular.events);
    assert(one.stats.input_bytes == bytes.size());
    assert(bytewise.stats.input_bytes == bytes.size());
    assert(irregular.stats.input_bytes == bytes.size());
    assert(one.stats.events == one.events.size());
    assert(one.stats.events > 1);

    assert(bytewise.stats.peak_buffered_bytes < 1024);
    assert(irregular.stats.peak_buffered_bytes < 1024);

    const auto tail = ingest("abc", 1, 16, 16);
    assert(tail.events.size() == 1);
    assert(tail.events[0].find("\"byte_offset\":0") != std::string::npos);
    assert(tail.events[0].find("\"byte_length\":3") != std::string::npos);

    return 0;
}
