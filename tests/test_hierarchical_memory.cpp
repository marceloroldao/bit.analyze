#include "bit_analyze/hierarchical_memory.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace bit_analyze;

    HierarchicalMemory memory;

    const std::vector<std::uint8_t> data{
        0x41, 0x42, 0x41, 0x42,
        0x41, 0x42, 0x41, 0x42,
        0x00, 0x00, 0x00, 0x00
    };

    const auto encoded = memory.encode(data, 3);
    const auto decoded = memory.decode(encoded.trail);

    assert(decoded == data);
    assert(memory.relation_count() > 0);
    assert(encoded.trail.size() < data.size());

    const auto relation_count_before = memory.relation_count();
    const auto encoded_again = memory.encode(data, 3);
    const auto decoded_again = memory.decode(encoded_again.trail);

    assert(decoded_again == data);
    assert(memory.relation_count() == relation_count_before);

    std::cout << "PASS: lossless hierarchical encode/decode\n";
    std::cout << "Input bytes: " << data.size() << "\n";
    std::cout << "Trail symbols: " << encoded.trail.size() << "\n";
    std::cout << "Relations: " << memory.relation_count() << "\n";

    return 0;
}
