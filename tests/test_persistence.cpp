#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/persistence.hpp"

#include <cassert>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace bit_analyze;

    std::vector<std::uint8_t> data;
    const std::vector<std::uint8_t> motif{'A','B','A','B','0','0','X','Y'};
    for (std::size_t i = 0; i < 4096; ++i) data.push_back(motif[i % motif.size()]);

    AdaptiveMemory memory;
    memory.learn_online(data, 32, 3, 1.5, 0.001);
    const auto encoded = memory.encode(data);
    assert(memory.decode(encoded.trail) == data);

    const std::string path = "bit_analyze_test_snapshot.bin";
    save_snapshot(MemorySnapshot{1, memory.rules(), {encoded.trail}}, path);

    const auto snapshot = load_snapshot(path);
    AdaptiveMemory restored;
    restored.load_rules(snapshot.rules);

    assert(snapshot.trails.size() == 1);
    assert(restored.decode(snapshot.trails.front()) == data);
    assert(restored.rules().size() == memory.rules().size());

    std::remove(path.c_str());
    std::cout << "PASS: persistence round-trip\n";
    return 0;
}
