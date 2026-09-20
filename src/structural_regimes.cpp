#include "bit_analyze/structural_regimes.hpp"
#include <limits>
namespace bit_analyze {
StructuralRegimeMatch StructuralRegimeSet::classify(StructuralChangeVector v) const noexcept{
 StructuralRegimeMatch best{}; double score=std::numeric_limits<double>::infinity();
 for(std::size_t i=0;i<regimes_.size();++i){auto d=regimes_[i].deviation(v);double s=d.extent+d.intensity;if(s<score){score=s;best={i,d,d.extent<=max_deviation_&&d.intensity<=max_deviation_};}}
 return best;
}
std::size_t StructuralRegimeSet::create_regime(StructuralChangeVector seed){regimes_.emplace_back();regimes_.back().observe(seed);return regimes_.size()-1;}
bool StructuralRegimeSet::observe(std::size_t index,StructuralChangeVector v) noexcept{if(index>=regimes_.size())return false;return regimes_[index].observe_if_consistent(v,max_deviation_).accepted;}
}
