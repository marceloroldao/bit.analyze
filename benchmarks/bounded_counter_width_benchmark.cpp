#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <vector>
using namespace bit_analyze;
namespace {
std::vector<StructuralEvent> events_for(const std::vector<std::uint8_t>&d){HierarchicalMemory m;StructuralExtractor x(m,2);StructuralStream s(x,128,128);std::vector<StructuralEvent>o;for(std::size_t p=0;p<d.size();){auto n=std::min<std::size_t>(17,d.size()-p);auto e=s.push(d.data()+p,n,"blind");o.insert(o.end(),e.begin(),e.end());p+=n;}auto e=s.flush("blind");o.insert(o.end(),e.begin(),e.end());return o;}
std::vector<std::uint8_t> base_data(){std::vector<std::uint8_t>v(8192);for(std::size_t i=0;i<v.size();++i)v[i]=static_cast<std::uint8_t>(((i%97)*13+(i/97)%31)&0xff);return v;}
std::vector<std::uint8_t> mutate(const std::vector<std::uint8_t>&in,double f){auto v=in;std::mt19937 rng(0xB17A2026);std::vector<std::size_t>idx(v.size());for(std::size_t i=0;i<idx.size();++i)idx[i]=i;std::shuffle(idx.begin(),idx.end(),rng);auto n=std::min(v.size(),static_cast<std::size_t>(std::ceil(v.size()*f)));for(std::size_t i=0;i<n;++i)v[idx[i]]^=static_cast<std::uint8_t>(1u<<(i%8));return v;}
BoundedStructuralFingerprint quantize(BoundedStructuralFingerprint f,unsigned bits,std::size_t& saturated){const std::uint64_t mx=bits==64?std::numeric_limits<std::uint64_t>::max():((std::uint64_t{1}<<bits)-1);saturated=0;auto q=[&](std::vector<std::uint64_t>&v){for(auto&x:v)if(x>mx){x=mx;++saturated;}};q(f.frequency);q(f.regional_frequency);q(f.transitions);return f;}
}
int main(){auto base=base_data();auto re=events_for(base);auto v0ref=make_structural_fingerprint(re,base.size(),8);BoundedStructuralFingerprintConfig cfg;cfg.frequency_buckets=4096;cfg.transition_buckets=4096;cfg.regions=8;auto rawref=make_bounded_structural_fingerprint(re,base.size(),cfg);std::cout<<"bounded_counter_width_4096_v0\nbits,mutation_fraction,mutated_bytes,v0_combined,bounded_combined,absolute_error,saturated_ref,saturated_candidate,logical_storage_bytes\n"<<std::fixed<<std::setprecision(6);for(unsigned bits:{8u,16u,32u,64u}){std::size_t sr=0;auto ref=quantize(rawref,bits,sr);for(double f:{0.0,1.0/double(base.size()),0.001,0.01,0.05,0.10,0.25,0.50}){auto d=mutate(base,f);auto e=events_for(d);auto v0=compare_structural_fingerprints(v0ref,make_structural_fingerprint(e,d.size(),8)).combined;std::size_t sc=0;auto q=quantize(make_bounded_structural_fingerprint(e,d.size(),cfg),bits,sc);auto b=compare_bounded_structural_fingerprints(ref,q).combined;auto changed=f==0?0:static_cast<std::size_t>(std::ceil(base.size()*f));auto counters=cfg.frequency_buckets+cfg.regions*cfg.frequency_buckets+cfg.transition_buckets;auto bytes=(counters*bits+7)/8;std::cout<<bits<<','<<f<<','<<changed<<','<<v0<<','<<b<<','<<std::abs(v0-b)<<','<<sr<<','<<sc<<','<<bytes<<'\n';}}}
