#include "bit_analyze/hierarchical_memory.hpp"
#include "bit_analyze/hierarchical_persistence.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace bit_analyze;

static std::vector<std::uint8_t> bytes(const std::string& value) {
    return std::vector<std::uint8_t>(value.begin(), value.end());
}

int main() {
    namespace fs = std::filesystem;
    const auto path = fs::temp_directory_path() / "bit-analyze-hierarchy-state.bin";
    std::error_code ec;
    fs::remove(path, ec);

    HierarchicalMemory original;
    const auto first = original.encode(bytes("ABABABABABABABAB"), 3);
    assert(original.relation_count() > 0);
    const auto before = original.relations();

    save_hierarchical_state(original, path.string());

    HierarchicalMemory restored;
    load_hierarchical_state(path.string(), restored);
    assert(restored.relation_count() == original.relation_count());
    assert(restored.relations().size() == before.size());

    for (std::size_t i = 0; i < before.size(); ++i) {
        assert(restored.relations()[i].id == before[i].id);
        assert(restored.relations()[i].left == before[i].left);
        assert(restored.relations()[i].right == before[i].right);
        assert(restored.relations()[i].layer == before[i].layer);
        assert(restored.relations()[i].ref_count == before[i].ref_count);
    }

    const auto second = restored.encode(bytes("ABABABABABABABAB"), 3);
    assert(second.trail == first.trail);
    assert(restored.relation_count() == original.relation_count());

    {
        std::fstream io(path, std::ios::binary | std::ios::in | std::ios::out);
        assert(io);
        io.seekg(-1, std::ios::end);
        char last = 0;
        io.read(&last, 1);
        last ^= 0x5a;
        io.seekp(-1, std::ios::end);
        io.write(&last, 1);
    }
    bool checksum_failed = false;
    try {
        HierarchicalMemory corrupted;
        load_hierarchical_state(path.string(), corrupted);
    } catch (const std::runtime_error&) {
        checksum_failed = true;
    }
    assert(checksum_failed);

    bool invalid_restore_failed = false;
    try {
        HierarchicalMemory invalid;
        invalid.restore_relations({RelationNode{257, 1, 2, 1, 1}});
    } catch (const std::invalid_argument&) {
        invalid_restore_failed = true;
    }
    assert(invalid_restore_failed);

    fs::remove(path, ec);
    return 0;
}
