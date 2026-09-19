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
std::vector<StructuralEvent> events_for(const std::vector<std::uint8_t>&d,std::size_t chunk){HierarchicalMemory m;StructuralExtractor x(m,2);StructuralStream s(x,128,128);std::vector<StructuralEvent>o;for(std::size_t p=0;p<d.size();){auto n=std::min(chunk,d.size()-p);auto e=s.push(d.data()+p,n,"blind");o.insert(o.end(),e.begin(),e.end());p+=n;}auto e=s.flush("blind");o.insert(o.end(),e.begin(),e.end());return o;}
std::vector<std::uint8_t> base_data(){std::vector<std::uint8_t>v(8192);for(std::size_t i=0;i<v.size();++i)v[i]=static_cast<std::uint8_t>(((i%97)*13+(i/97)%31)&0xff);return v;}
std::vector<std::uint8_t> mutate(const std::vector<std::uint8_t>&in,double f){auto v=in;std::mt19937 rng(0xB17A2026);std::vector<std::size_t>idx(v.size());for(std::size_t i=0;i<idx.size();++i)idx[i]=i;std::shuffle(idx.begin(),idx.end(),rng);auto n=std::min(v.size(),static_cast<std::size_t>(std::ceil(v.size()*f)));for(std::size_t i=0;i<n;++i)v[idx[i]]^=static_cast<std::uint8_t>(1u<<(i%8));return v;}
struct Multi{std::vector<BoundedStructuralFingerprint> rows;};
Multi make_multi(const std::vector<StructuralEvent>&e,std::uint64_t bytes,std::size_t rows,std::size_t buckets){Multi m;for(std::size_t r=0;r<rows;++r){BoundedStructuralFingerprintConfig c;c.frequency_buckets=buckets;c.transition_buckets=buckets;c.regions=8;c.hash_seed=0xB17A2026ULL+(r*0x9e3779b97f4a7c15ULL);m.rows.push_back(make_bounded_structural_fingerprint(e,bytes,c));}return m;}
double compare_multi(const Multi&a,const Multi&b){double sum=0;for(std::size_t i=0;i<a.rows.size();++i)sum+=compare_bounded_structural_fingerprints(a.rows[i],b.rows[i]).combined;return sum/a.rows.size();}
std::size_t storage(const Multi&m){std::size_t n=0;for(const auto&r:m.rows)n+=r.storage_bytes();return n;}
}
int main(){auto base=base_data();auto re=events_for(base,17);auto v0ref=make_structural_fingerprint(re,base.size(),8);std::cout<<"bounded_multihash_same_budget_v0\nrows,buckets_per_row,mutation_fraction,mutated_bytes,v0_combined,multihash_combined,absolute_error,storage_bytes\n"<<std::fixed<<std::setprecision(6);for(auto shape:std::vector<std::pair<std::size_t,std::size_t>>{{1,1024},{2,512},{4,256},{8,128}}){auto mr=make_multi(re,base.size(),shape.first,shape.second);for(double f:{0.0,1.0/double(base.size()),0.001,0.01,0.05,0.10,0.25,0.50}){auto d=mutate(base,f);auto e=events_for(d,17);auto v0=compare_structural_fingerprints(v0ref,make_structural_fingerprint(e,d.size(),8)).combined;auto mm=compare_multi(mr,make_multi(e,d.size(),shape.first,shape.second));auto changed=f==0?0:static_cast<std::size_t>(std::ceil(base.size()*f));std::cout<<shape.first<<','<<shape.second<<','<<f<<','<<changed<<','<<v0<<','<<mm<<','<<std::abs(v0-mm)<<','<<storage(mr)<<'\n';}}}
