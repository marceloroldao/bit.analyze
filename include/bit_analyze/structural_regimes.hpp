#pragma once
#include "bit_analyze/structural_baseline.hpp"
#include <cstddef>
#include <vector>
namespace bit_analyze {
struct StructuralRegimeMatch { std::size_t index{}; StructuralChangeVector deviation{}; bool matched{}; };
class StructuralRegimeSet {
public:
 explicit StructuralRegimeSet(double max_deviation=3.0):max_deviation_(max_deviation){}
 StructuralRegimeMatch classify(StructuralChangeVector v) const noexcept;
 std::size_t create_regime(StructuralChangeVector seed);
 bool observe(std::size_t index,StructuralChangeVector v) noexcept;
 std::size_t size() const noexcept{return regimes_.size();}
 const StructuralBaseline& regime(std::size_t i) const{return regimes_.at(i);}
private:
 double max_deviation_;
 std::vector<StructuralBaseline> regimes_;
};
}
