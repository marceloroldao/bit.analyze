#include "bit_analyze/hierarchical_memory.hpp"
#include "bit_analyze/structural_stream.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace bit_analyze;
static std::vector<std::uint8_t> input(){std::vector<std::uint8_t>x(65536);for(std::size_t i=0;i<x.size();++i)x[i]=static_cast<std::uint8_t>((i%16)<8?0xAA:0x55);return x;}
static std::vector<StructuralEvent> run(const std::vector<std::uint8_t>&x){HierarchicalMemory m;StructuralExtractor e(m,2);StructuralStream s(e,128,128);std::vector<StructuralEvent> out;for(std::size_t p=0;p<x.size();p+=257){auto n=std::min<std::size_t>(257,x.size()-p);auto v=s.push(x.data()+p,n,"propagation");out.insert(out.end(),v.begin(),v.end());}auto v=s.flush("propagation");out.insert(out.end(),v.begin(),v.end());return out;}
int main(){auto a=input(),b=a;b[32768]^=0x5B;auto x=run(a),y=run(b);auto n=std::min(x.size(),y.size());std::size_t changed=0,first=n,last=0;for(std::size_t i=0;i<n;++i)if(x[i].signature!=y[i].signature||x[i].relation_ids!=y[i].relation_ids){++changed;first=std::min(first,i);last=i;}std::cout<<"single_byte_propagation_v0\nevents_base,events_mutated,changed_events,first_changed,last_changed,changed_fraction\n"<<x.size()<<","<<y.size()<<","<<changed<<","<<(first==n?0:first)<<","<<last<<","<<(n?double(changed)/double(n):0.0)<<"\n";return changed>0?0:1;}
