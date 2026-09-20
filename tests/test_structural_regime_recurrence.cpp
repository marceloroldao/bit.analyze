#include "bit_analyze/structural_baseline.hpp"
#include <iostream>
using namespace bit_analyze;
int main(){
 StructuralBaseline a,b;
 const StructuralChangeVector A[]={{.018,.42},{.017,.43},{.019,.41},{.0185,.425},{.0175,.415},{.0182,.422}};
 for(auto v:A)a.observe(v);
 const StructuralChangeVector B[]={{.78,.19},{.77,.20},{.79,.18},{.775,.195},{.785,.185},{.78,.192}};
 std::size_t rejected=0;
 for(auto v:B) if(!a.observe_if_consistent(v,3.0).accepted){++rejected;b.observe(v);}
 auto b_local=b.deviation({.782,.191});
 auto a_view=a.deviation({.782,.191});
 auto return_a=a.deviation({.0181,.421});
 std::cout<<"structural_regime_recurrence_v0\nrejected_by_A,B_samples,A_view_extent,A_view_intensity,B_local_extent,B_local_intensity,A_return_extent,A_return_intensity\n"
 <<rejected<<","<<b.state().samples<<","<<a_view.extent<<","<<a_view.intensity<<","<<b_local.extent<<","<<b_local.intensity<<","<<return_a.extent<<","<<return_a.intensity<<"\n";
 if(rejected!=6||b.state().samples!=6)return 1;
 if(b_local.extent>=a_view.extent||b_local.intensity>=a_view.intensity)return 1;
 if(a.state().samples!=6)return 1;
 return 0;
}
