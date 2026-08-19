#include "bit_analyze/interleaving.hpp"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

int main() {
    for (std::size_t n : {1U, 2U, 63U, 64U, 65U, 127U, 128U, 4096U}) {
        const auto order = bit_analyze::interleaved_order(n, 64);
        const auto inverse = bit_analyze::inverse_interleaved_order(n, 64);
        assert(order.size() == n);
        assert(inverse.size() == n);

        std::vector<bool> seen(n, false);
        for (std::size_t physical = 0; physical < n; ++physical) {
            const auto logical = order[physical];
            assert(logical < n);
            assert(!seen[logical]);
            seen[logical] = true;
            assert(inverse[logical] == physical);
        }
    }

    std::cout << "PASS: reversible interleaving mapping\n";
    return 0;
}
