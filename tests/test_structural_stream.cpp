#include "bit_analyze/structural_stream.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

using namespace bit_analyze;

static std::vector<StructuralEvent> run_stream(const std::vector<std::uint8_t>& data,
                                               const std::vector<std::size_t>& chunks,
                                               std::size_t window, std::size_t hop) {
    HierarchicalMemory memory;
    StructuralExtractor extractor(memory, 2);
    StructuralStream stream(extractor, window, hop);
    std::vector<StructuralEvent> out;
    std::size_t pos = 0;
    std::size_t ci = 0;
    while (pos < data.size()) {
        const auto n = std::min(chunks[ci++ % chunks.size()], data.size() - pos);
        auto batch = stream.push(data.data() + pos, n, "sample");
        out.insert(out.end(), batch.begin(), batch.end());
        pos += n;
        assert(stream.buffered_bytes() < window);
    }
    auto tail = stream.flush("sample");
    out.insert(out.end(), tail.begin(), tail.end());
    return out;
}

static void assert_equivalent(const std::vector<StructuralEvent>& a,
                              const std::vector<StructuralEvent>& b) {
    assert(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        assert(a[i].sequence == b[i].sequence);
        assert(a[i].byte_offset == b[i].byte_offset);
        assert(a[i].byte_length == b[i].byte_length);
        assert(a[i].trail == b[i].trail);
        assert(a[i].relation_ids == b[i].relation_ids);
        assert(a[i].signature == b[i].signature);
    }
}

int main() {
    std::vector<std::uint8_t> data;
    for (std::size_t i = 0; i < 4097; ++i)
        data.push_back(static_cast<std::uint8_t>((i * 17 + (i / 11)) & 0xff));

    const auto one_chunk = run_stream(data, {data.size()}, 128, 128);
    const auto irregular = run_stream(data, {1, 7, 31, 3, 257, 19}, 128, 128);
    assert_equivalent(one_chunk, irregular);

    HierarchicalMemory direct_memory;
    StructuralExtractor direct(direct_memory, 2);
    std::vector<StructuralEvent> direct_events;
    std::uint64_t seq = 0;
    for (std::size_t offset = 0; offset < data.size(); offset += 128) {
        const auto n = std::min<std::size_t>(128, data.size() - offset);
        std::vector<std::uint8_t> window(data.begin() + offset, data.begin() + offset + n);
        direct_events.push_back(direct.extract(window, "sample", seq++, offset));
    }
    assert_equivalent(direct_events, irregular);

    assert(!irregular.empty());
    const auto json = irregular.front().to_json();
    assert(json.find("\"version\":1") != std::string::npos);
    assert(json.find("\"byte_offset\":0") != std::string::npos);
    assert(json.find("\"trail\":[") != std::string::npos);

    // Overlap mode: chunking must still be irrelevant and offsets follow hop_size.
    const auto overlap_a = run_stream(data, {4097}, 96, 32);
    const auto overlap_b = run_stream(data, {2, 5, 13, 127}, 96, 32);
    assert_equivalent(overlap_a, overlap_b);
    for (std::size_t i = 1; i + 1 < overlap_b.size(); ++i)
        assert(overlap_b[i].byte_offset - overlap_b[i - 1].byte_offset == 32);

    return 0;
}
