#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>
using namespace bit_analyze;
namespace {
std::vector<StructuralEvent> events_for(const std::vector<std::uint8_t>&data,std::size_t chunk){HierarchicalMemory memory;StructuralExtractor extractor(memory,2);StructuralStream stream(extractor,128,128);std::vector<StructuralEvent>out;std::size_t pos=0;while(pos<data.size()){auto n=std::min(chunk,data.size()-pos);auto e=stream.push(data.data()+pos,n,"blind");out.insert(out.end(),e.begin(),e.end());pos+=n;}auto e=stream.flush("blind");out.insert(out.end(),e.begin(),e.end());return out;}
std::vector<std::uint8_t> base_data(){std::vector<std::uint8_t>v(8192);for(std::size_t i=0;i<v.size();++i)v[i]=static_cast<std::uint8_t>(((i%97)*13+(i/97)%31)&0xff);return v;}
std::vector<std::uint8_t> mutate(const std::vector<std::uint8_t>&in,double fraction){auto v=in;std::mt19937 rng(0xB17A2026);std::vector<std::size_t>idx(v.size());for(std::size_t i=0;i<idx.size();++i)idx[i]=i;std::shuffle(idx.begin(),idx.end(),rng);auto count=static_cast<std::size_t>(std::ceil(v.size()*fraction));count=std::min(count,v.size());for(std::size_t i=0;i<count;++i)v[idx[i]]^=static_cast<std::uint8_t>(1u<<(i%8));return v;}
bool equivalent(const BoundedStructuralFingerprint&a,const BoundedStructuralFingerprint&b){return a.total_bytes==b.total_bytes&&a.event_count==b.event_count&&a.config.frequency_buckets==b.config.frequency_buckets&&a.config.transition_buckets==b.config.transition_buckets&&a.config.regions==b.config.regions&&a.config.hash_seed==b.config.hash_seed&&a.frequency==b.frequency&&a.regional_frequency==b.regional_frequency&&a.transitions==b.transitions;}
}
int main(){const auto base=base_data();const auto ref_events=events_for(base,17);const auto exact_ref=make_structural_fingerprint(ref_events,base.size(),8);std::cout<<"bounded_structural_fingerprint_v1_capacity_sweep\ncapacity,mutation_fraction,mutated_bytes,v0_combined,v1_combined,absolute_error,chunk_invariant,storage_bytes\n"<<std::fixed<<std::setprecision(6);for(std::size_t cap:{64u,128u,256u,512u,1024u}){BoundedStructuralFingerprintConfig cfg;cfg.frequency_buckets=cap;cfg.transition_buckets=cap;cfg.regions=8;const auto bounded_ref=make_bounded_structural_fingerprint(ref_events,base.size(),cfg);for(double f:{0.0,1.0/double(base.size()),0.001,0.01,0.05,0.10,0.25,0.50}){auto d=mutate(base,f);auto e17=events_for(d,17),e257=events_for(d,257);auto exact=make_structural_fingerprint(e17,d.size(),8);auto a=make_bounded_structural_fingerprint(e17,d.size(),cfg),b=make_bounded_structural_fingerprint(e257,d.size(),cfg);auto v0=compare_structural_fingerprints(exact_ref,exact).combined;auto v1=compare_bounded_structural_fingerprints(bounded_ref,a).combined;auto changed=f==0.0?0:static_cast<std::size_t>(std::ceil(base.size()*f));std::cout<<cap<<','<<f<<','<<changed<<','<<v0<<','<<v1<<','<<std::abs(v0-v1)<<','<<(equivalent(a,b)?1:0)<<','<<a.storage_bytes()<<'\n';}}}
