#include "bit_analyze/structural_baseline.hpp"
#include <iostream>
using namespace bit_analyze;
int main(){
 StructuralBaseline b;
 const StructuralChangeVector normal[]={{0.018,0.42},{0.017,0.43},{0.019,0.41},{0.0185,0.425},{0.0175,0.415},{0.0182,0.422}};
 for(auto v:normal)b.observe(v);
 const auto before=b.state();
 auto steady=b.deviation({0.0181,0.421});
 auto change=b.deviation({0.78,0.19});
 auto returned=b.deviation({0.0179,0.423});
 std::cout<<"structural_baseline_temporal_v0\n"
 <<"samples,mean_extent,mean_intensity,steady_extent,steady_intensity,change_extent,change_intensity,return_extent,return_intensity\n"
 <<before.samples<<","<<before.mean_extent<<","<<before.mean_intensity<<","
 <<steady.extent<<","<<steady.intensity<<","<<change.extent<<","<<change.intensity<<","
 <<returned.extent<<","<<returned.intensity<<"\n";
 if(before.samples!=6)return 1;
 if(change.extent<=steady.extent||change.extent<=returned.extent)return 1;
 if(change.intensity<=steady.intensity||change.intensity<=returned.intensity)return 1;
 if(b.state().samples!=before.samples)return 1;
 return 0;
}
