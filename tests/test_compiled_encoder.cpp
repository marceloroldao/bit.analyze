#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/compiled_encoder.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace bit_analyze;

    std::vector<std::vector<std::uint8_t>> corpus{
        {'A','B','C','D','A','B','C','D','0','0','0','0','X','Y','X','Y'},
        {'A','B','C','D','A','B','C','D','0','0','0','0','X','Y','X','Y'},
        {'m','n','o','p','m','n','o','p','1','2','3','4','1','2','3','4'}
    };

    AdaptiveMemory memory;
    memory.train(corpus, 64, 2, 1.0, 0.0);

    const std::vector<std::uint8_t> probe{
        'X','Y','A','B','C','D','A','B','C','D','0','0','0','0','X','Y'
    };

    const auto canonical = memory.encode(probe);
    assert(memory.decode(canonical.trail) == probe);

    CompiledEncoder compiled(memory);
    const auto fast = compiled.encode(probe);
    assert(memory.decode(fast.trail) == probe);

    std::cout << "PASS: compiled encoder lossless\n";
    std::cout << "rules=" << memory.rule_count() << '\n';
    std::cout << "trie_nodes=" << compiled.trie_node_count() << '\n';
    std::cout << "canonical_trail=" << canonical.trail.size() << '\n';
    std::cout << "compiled_trail=" << fast.trail.size() << '\n';
    std::cout << "matched_bytes=" << fast.matched_bytes << '\n';

    return 0;
}
