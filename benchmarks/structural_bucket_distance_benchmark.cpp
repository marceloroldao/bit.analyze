#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include "bit_analyze/hierarchical_memory.hpp"
#include "bit_analyze/structural_stream.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace bit_analyze;
static std::vector<std::uint8_t> base(std::size_t n){std::vector<std::uint8_t>x(n);for(std::size_t i=0;i<n;++i)x[i]=static_cast<std::uint8_t>((i%16)<8?0xAA:0x55);return x;}
static std::vector<std::uint8_t> mutate(std::vector<std::uint8_t>x,std::size_t every){if(every)for(std::size_t i=0;i<x.size();i+=every)x[i]^=static_cast<std::uint8_t>(0x5B+(i&31));return x;}
static RollingStructuralFingerprint8 run(const std::vector<std::uint8_t>&x){RollingStructuralFingerprintConfig c;c.bounded.frequency_buckets=256;c.bounded.transition_buckets=256;c.bounded.regions=8;c.region_span_bytes=4096;HierarchicalMemory m;StructuralExtractor e(m,2);StructuralStream s(e,128,128);RollingStructuralFingerprintAccumulator8 a(c);RollingStructuralStream8 p(s,a);p.push(x,"distance");p.flush("distance");return p.fingerprint();}
struct D{double changed_fraction,magnitude_changed,score;};
static D distance(const std::vector<std::uint8_t>&a,const std::vector<std::uint8_t>&b){std::size_t changed=0;double mag=0;for(std::size_t i=0;i<a.size();++i)if(a[i]!=b[i]){++changed;mag+=std::abs(int(a[i])-int(b[i]))/255.0;}double f=a.empty()?0:double(changed)/a.size();double m=changed?mag/changed:0;return {f,m,f*m};}
int main(){auto a=base(65536);auto f0=run(a);const std::size_t steps[]={0,1024,256,64,16,4};std::cout<<"structural_bucket_distance_v0\nmutation_every,changed_fraction,magnitude_changed,score\n";double zero=-1,maxscore=0;for(auto e:steps){auto f=run(mutate(a,e));auto d=distance(f0.regional_frequency,f.regional_frequency);std::cout<<e<<","<<d.changed_fraction<<","<<d.magnitude_changed<<","<<d.score<<"\n";if(e==0)zero=d.score;else maxscore=std::max(maxscore,d.score);}return zero==0&&maxscore>0?0:1;}
