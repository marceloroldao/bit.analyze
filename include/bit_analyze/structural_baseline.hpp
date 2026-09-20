#pragma once
#include <cstddef>
namespace bit_analyze {
struct StructuralChangeVector { double extent{}; double intensity{}; };
struct StructuralBaselineState {
 std::size_t samples{};
 double mean_extent{}, mean_intensity{};
 double m2_extent{}, m2_intensity{};
};
struct StructuralBaselineUpdate { StructuralChangeVector deviation{}; bool accepted{}; };
class StructuralBaseline {
public:
 void observe(StructuralChangeVector v) noexcept;
 const StructuralBaselineState& state() const noexcept { return state_; }
 StructuralChangeVector deviation(StructuralChangeVector v) const noexcept;
 StructuralBaselineUpdate observe_if_consistent(StructuralChangeVector v, double max_deviation) noexcept;
private:
 StructuralBaselineState state_{};
};
}
