#include "bit_analyze/structural_fingerprint.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>
using namespace bit_analyze;
namespace {
std::vector<StructuralEvent> events_for(const std::vector<std::uint8_t>& data,std::size_t chunk){
 HierarchicalMemory memory; StructuralExtractor extractor(memory,2); StructuralStream stream(extractor,128,128); std::vector<StructuralEvent> out; std::size_t pos=0;
 while(pos<data.size()){auto n=std::min(chunk,data.size()-pos);auto e=stream.push(data.data()+pos,n,"blind");out.insert(out.end(),e.begin(),e.end());pos+=n;} auto e=stream.flush("blind");out.insert(out.end(),e.begin(),e.end());return out;
}
StructuralFingerprint fingerprint(const std::vector<std::uint8_t>& d,std::size_t chunk){return make_structural_fingerprint(events_for(d,chunk),d.size(),8);}
std::vector<std::uint8_t> base_data(){std::vector<std::uint8_t> v(8192);for(std::size_t i=0;i<v.size();++i)v[i]=static_cast<std::uint8_t>(((i%97)*13+(i/97)%31)&0xff);return v;}
std::vector<std::uint8_t> mutate(const std::vector<std::uint8_t>& in,double fraction){auto v=in;std::mt19937 rng(0xB17A2026);std::vector<std::size_t> idx(v.size());for(std::size_t i=0;i<idx.size();++i)idx[i]=i;std::shuffle(idx.begin(),idx.end(),rng);auto count=static_cast<std::size_t>(std::ceil(v.size()*fraction));count=std::min(count,v.size());for(std::size_t i=0;i<count;++i)v[idx[i]]^=static_cast<std::uint8_t>(1u<<(i%8));return v;}
bool equivalent(const StructuralFingerprint&a,const StructuralFingerprint&b){return a.total_bytes==b.total_bytes&&a.event_count==b.event_count&&a.relation_frequency==b.relation_frequency&&a.regional_frequency==b.regional_frequency&&a.transitions==b.transitions;}
}
int main(){const auto base=base_data();const auto ref=fingerprint(base,17);std::cout<<"structural_fingerprint_v0_gradient\nmutation_fraction,mutated_bytes,frequency,regional,transitions,combined,chunk_invariant\n"<<std::fixed<<std::setprecision(6);
 for(double f:{0.0,1.0/double(base.size()),0.001,0.01,0.05,0.10,0.25,0.50}){auto d=mutate(base,f);auto a=fingerprint(d,17),b=fingerprint(d,257);auto s=compare_structural_fingerprints(ref,a);auto changed=f==0.0?0:static_cast<std::size_t>(std::ceil(base.size()*f));std::cout<<f<<','<<changed<<','<<s.frequency<<','<<s.regional<<','<<s.transitions<<','<<s.combined<<','<<(equivalent(a,b)?1:0)<<'\n';}
}
