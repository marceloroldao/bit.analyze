#include "bit_analyze/structural_baseline.hpp"
#include <cmath>
namespace bit_analyze {
void StructuralBaseline::observe(StructuralChangeVector v) noexcept {
 ++state_.samples;
 const double de=v.extent-state_.mean_extent;
 state_.mean_extent+=de/static_cast<double>(state_.samples);
 state_.m2_extent+=de*(v.extent-state_.mean_extent);
 const double di=v.intensity-state_.mean_intensity;
 state_.mean_intensity+=di/static_cast<double>(state_.samples);
 state_.m2_intensity+=di*(v.intensity-state_.mean_intensity);
}
StructuralChangeVector StructuralBaseline::deviation(StructuralChangeVector v) const noexcept {
 if(state_.samples<2)return {};
 const double se=std::sqrt(state_.m2_extent/static_cast<double>(state_.samples-1));
 const double si=std::sqrt(state_.m2_intensity/static_cast<double>(state_.samples-1));
 return {se>0?std::abs(v.extent-state_.mean_extent)/se:std::abs(v.extent-state_.mean_extent),
         si>0?std::abs(v.intensity-state_.mean_intensity)/si:std::abs(v.intensity-state_.mean_intensity)};
}
}
