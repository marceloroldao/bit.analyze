#pragma once
#include <cstddef>
namespace bit_analyze {
struct StructuralChangeVector { double extent{}; double intensity{}; };
struct StructuralBaselineState {
 std::size_t samples{};
 double mean_extent{}, mean_intensity{};
 double m2_extent{}, m2_intensity{};
};
class StructuralBaseline {
public:
 void observe(StructuralChangeVector v) noexcept;
 const StructuralBaselineState& state() const noexcept { return state_; }
 StructuralChangeVector deviation(StructuralChangeVector v) const noexcept;
private:
 StructuralBaselineState state_{};
};
}
