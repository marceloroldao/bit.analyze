#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/hierarchical_memory.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

static Bytes bytes(const std::string& s) {
    return Bytes(s.begin(), s.end());
}

struct CostResult {
    std::size_t adaptive_trail{};
    std::size_t fixed_trail{};
    std::size_t fixed_new_relations{};
    std::size_t adaptive_cost{};
    std::size_t fixed_cost{};
};

static CostResult compare_probe(bit_analyze::AdaptiveMemory& adaptive,
                                bit_analyze::HierarchicalMemory& fixed,
                                const Bytes& probe) {
    const auto a = adaptive.encode(probe);
    assert(adaptive.decode(a.trail) == probe);

    const auto before = fixed.relation_count();
    const auto f = fixed.encode(probe, 3);
    const auto after = fixed.relation_count();
    assert(fixed.decode(f.trail) == probe);

    const auto new_rel = after - before;

    // Inference cost proxy:
    // - each trail symbol costs 1 unit
    // - each newly-created fixed relation costs 2 units for its operands
    // Adaptive inference uses a frozen dictionary and creates no rules.
    return {
        a.trail.size(),
        f.trail.size(),
        new_rel,
        a.trail.size(),
        f.trail.size() + 2 * new_rel
    };
}

static void train_both(bit_analyze::AdaptiveMemory& adaptive,
                       bit_analyze::HierarchicalMemory& fixed,
                       const std::vector<Bytes>& train) {
    adaptive.train(train, 64, 2);
    for (const auto& sample : train) {
        const auto encoded = fixed.encode(sample, 3);
        assert(fixed.decode(encoded.trail) == sample);
    }
}

int main() {
    using namespace bit_analyze;

    const std::vector<Bytes> structured_train{
        bytes("ABABABABABAB"),
        bytes("ABABCDABABCD"),
        bytes("CDABABCDABAB")
    };

    {
        AdaptiveMemory adaptive;
        HierarchicalMemory fixed;
        train_both(adaptive, fixed, structured_train);

        const std::vector<Bytes> shifted{
            bytes("XABABCDY"),
            bytes("YYABABABZZ"),
            bytes("CDABABCD")
        };

        std::size_t adaptive_total = 0;
        std::size_t fixed_total = 0;
        for (const auto& probe : shifted) {
            const auto r = compare_probe(adaptive, fixed, probe);
            adaptive_total += r.adaptive_cost;
            fixed_total += r.fixed_cost;
        }

        std::cout << "shifted adaptive_cost=" << adaptive_total
                  << " fixed_cost=" << fixed_total << "\n";
        assert(adaptive_total < fixed_total);
    }

    {
        AdaptiveMemory adaptive;
        HierarchicalMemory fixed;
        train_both(adaptive, fixed, structured_train);

        const std::vector<Bytes> noisy{
            bytes("XABQBCDZ"),
            bytes("ABABRABABS"),
            bytes("TCDABUCD")
        };

        std::size_t adaptive_total = 0;
        std::size_t fixed_total = 0;
        for (const auto& probe : noisy) {
            const auto r = compare_probe(adaptive, fixed, probe);
            adaptive_total += r.adaptive_cost;
            fixed_total += r.fixed_cost;
        }

        std::cout << "noise adaptive_cost=" << adaptive_total
                  << " fixed_cost=" << fixed_total << "\n";
        assert(adaptive_total < fixed_total);
    }

    {
        std::mt19937 rng(1);
        std::uniform_int_distribution<int> dist(0, 255);

        std::vector<Bytes> train(3, Bytes(32));
        std::vector<Bytes> probes(3, Bytes(32));
        for (auto& sample : train) {
            for (auto& b : sample) b = static_cast<std::uint8_t>(dist(rng));
        }
        for (auto& sample : probes) {
            for (auto& b : sample) b = static_cast<std::uint8_t>(dist(rng));
        }

        AdaptiveMemory adaptive;
        HierarchicalMemory fixed;
        train_both(adaptive, fixed, train);

        // Random data should not create a large adaptive vocabulary.
        assert(adaptive.rule_count() <= 2);

        for (const auto& probe : probes) {
            const auto r = compare_probe(adaptive, fixed, probe);
            assert(r.adaptive_trail <= probe.size());
            assert(r.fixed_new_relations > 0);
        }
    }

    std::cout << "PASS: cost benchmark\n";
    return 0;
}
