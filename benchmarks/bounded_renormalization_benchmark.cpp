#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>
using namespace bit_analyze;
namespace {
std::vector<std::uint8_t> data(std::size_t n){std::vector<std::uint8_t>v(n);for(std::size_t i=0;i<n;++i)v[i]=static_cast<std::uint8_t>(((i%97)*13+(i/97)%31)&0xff);return v;}
std::vector<StructuralEvent> events_for(const std::vector<std::uint8_t>&d){HierarchicalMemory m;StructuralExtractor x(m,2);StructuralStream s(x,128,128);std::vector<StructuralEvent>o;for(std::size_t p=0;p<d.size();){auto n=std::min<std::size_t>(4096,d.size()-p);auto e=s.push(d.data()+p,n,"renorm");o.insert(o.end(),e.begin(),e.end());p+=n;}auto e=s.flush("renorm");o.insert(o.end(),e.begin(),e.end());return o;}
void renorm(std::vector<std::uint64_t>&v,std::size_t&count){auto mx=*std::max_element(v.begin(),v.end());if(mx<255)return;for(auto&x:v)x=(x+1)/2;++count;}
BoundedStructuralFingerprint q8(BoundedStructuralFingerprint f,std::size_t&passes){passes=0;auto q=[&](std::vector<std::uint64_t>&v){while(*std::max_element(v.begin(),v.end())>255)renorm(v,passes);};q(f.frequency);for(std::size_t r=0;r<f.config.regions;++r){auto b=f.regional_frequency.begin()+r*f.config.frequency_buckets,e=b+f.config.frequency_buckets;std::vector<std::uint64_t>tmp(b,e);q(tmp);std::copy(tmp.begin(),tmp.end(),b);}q(f.transitions);return f;}
BoundedStructuralFingerprint q16(BoundedStructuralFingerprint f){auto q=[](std::vector<std::uint64_t>&v){for(auto&x:v)x=std::min<std::uint64_t>(x,65535);};q(f.frequency);q(f.regional_frequency);q(f.transitions);return f;}
}
int main(){BoundedStructuralFingerprintConfig c;c.frequency_buckets=4096;c.transition_buckets=4096;c.regions=8;std::cout<<"bounded_renormalization_8bit_v0\nbytes,events,renorm_passes,similarity_8renorm_vs_64,similarity_16sat_vs_64,storage_8_bytes,storage_16_bytes\n"<<std::fixed<<std::setprecision(6);for(std::size_t n:{8192u,65536u,1048576u,16777216u}){auto d=data(n);auto e=events_for(d);auto raw=make_bounded_structural_fingerprint(e,n,c);std::size_t passes=0;auto a=q8(raw,passes),b=q16(raw);auto s8=compare_bounded_structural_fingerprints(raw,a).combined;auto s16=compare_bounded_structural_fingerprints(raw,b).combined;auto counters=c.frequency_buckets+c.regions*c.frequency_buckets+c.transition_buckets;std::cout<<n<<','<<e.size()<<','<<passes<<','<<s8<<','<<s16<<','<<counters<<','<<counters*2<<'\n';}}
