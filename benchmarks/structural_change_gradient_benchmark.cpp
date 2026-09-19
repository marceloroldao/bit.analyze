#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include "bit_analyze/hierarchical_memory.hpp"
#include "bit_analyze/structural_stream.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace bit_analyze;
static std::vector<std::uint8_t> base(std::size_t n){std::vector<std::uint8_t>x(n);for(std::size_t i=0;i<n;++i)x[i]=static_cast<std::uint8_t>((i%16)<8?0xAA:0x55);return x;}
static std::vector<std::uint8_t> mutate(std::vector<std::uint8_t>x,std::size_t every){if(every)for(std::size_t i=0;i<x.size();i+=every)x[i]^=static_cast<std::uint8_t>(0x5B+(i&31));return x;}
static RollingStructuralFingerprint8 run(const std::vector<std::uint8_t>&x){RollingStructuralFingerprintConfig c;c.bounded.frequency_buckets=256;c.bounded.transition_buckets=256;c.bounded.regions=8;c.region_span_bytes=4096;HierarchicalMemory m;StructuralExtractor e(m,2);StructuralStream s(e,128,128);RollingStructuralFingerprintAccumulator8 a(c);RollingStructuralStream8 p(s,a);p.push(x,"gradient");p.flush("gradient");return p.fingerprint();}
static double distance(const std::vector<std::uint8_t>&a,const std::vector<std::uint8_t>&b){std::uint64_t d=0,z=0;for(std::size_t i=0;i<a.size();++i){d+=a[i]>b[i]?a[i]-b[i]:b[i]-a[i];z+=std::max(a[i],b[i]);}return z?double(d)/double(z):0.0;}
int main(){auto a=base(65536);auto f0=run(a);const std::size_t steps[]={0,256,64,16,4};double prev=-1;std::cout<<"structural_change_gradient_v0\nmutation_every,regional_distance\n";for(auto e:steps){auto f=run(mutate(a,e));double d=distance(f0.regional_frequency,f.regional_frequency);std::cout<<e<<","<<d<<"\n";if(d+1e-12<prev)return 1;prev=d;}return prev>0?0:1;}
