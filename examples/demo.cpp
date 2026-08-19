#include "bit_analyze/hierarchical_memory.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace bit_analyze;

    const std::string text = "ABABABABABABABAB";
    const std::vector<std::uint8_t> data(text.begin(), text.end());

    HierarchicalMemory memory;
    const auto encoded = memory.encode(data, 4);
    const auto decoded = memory.decode(encoded.trail);

    std::cout << "bit.analyze demo\n";
    std::cout << "input bytes      : " << data.size() << "\n";
    std::cout << "trail symbols    : " << encoded.trail.size() << "\n";
    std::cout << "relations stored : " << memory.relation_count() << "\n";
    std::cout << "lossless         : " << (decoded == data ? "yes" : "no") << "\n";

    return decoded == data ? 0 : 1;
}
