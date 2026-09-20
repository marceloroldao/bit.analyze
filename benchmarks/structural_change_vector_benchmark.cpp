#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include "bit_analyze/hierarchical_memory.hpp"
#include "bit_analyze/structural_stream.hpp"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
using namespace bit_analyze;
static std::vector<std::uint8_t> corpus(std::size_t n,int kind){std::vector<std::uint8_t>x(n);std::uint32_t s=0x12345678u;for(std::size_t i=0;i<n;++i){if(kind==0)x[i]=static_cast<std::uint8_t>((i%16)<8?0xAA:0x55);else if(kind==1)x[i]=static_cast<std::uint8_t>((i*17+(i/97)*31)&255);else{s^=s<<13;s^=s>>17;s^=s<<5;x[i]=static_cast<std::uint8_t>(s);}}return x;}
static std::vector<std::uint8_t> mutate(std::vector<std::uint8_t>x,std::size_t every){if(every)for(std::size_t i=0;i<x.size();i+=every)x[i]^=static_cast<std::uint8_t>(0x5B+(i&31));return x;}
static RollingStructuralFingerprint8 run(const std::vector<std::uint8_t>&x){RollingStructuralFingerprintConfig c;c.bounded.frequency_buckets=256;c.bounded.transition_buckets=256;c.bounded.regions=8;c.region_span_bytes=4096;HierarchicalMemory m;StructuralExtractor e(m,2);StructuralStream s(e,128,128);RollingStructuralFingerprintAccumulator8 a(c);RollingStructuralStream8 p(s,a);p.push(x,"vector");p.flush("vector");return p.fingerprint();}
static void metric(const std::vector<std::uint8_t>&a,const std::vector<std::uint8_t>&b,double&extent,double&intensity){std::size_t changed=0;double mag=0;for(std::size_t i=0;i<a.size();++i)if(a[i]!=b[i]){++changed;mag+=std::abs(int(a[i])-int(b[i]))/255.0;}extent=a.empty()?0:double(changed)/a.size();intensity=changed?mag/changed:0;}
int main(){const char* names[]={"repetitive","structured","pseudo_random"};const std::size_t steps[]={0,1024,256,64,16,4};std::cout<<"structural_change_vector_v0\ncorpus,mutation_every,extent,intensity\n";bool detected=false;for(int k=0;k<3;++k){auto a=corpus(65536,k);auto f0=run(a);for(auto e:steps){auto f=run(mutate(a,e));double extent,intensity;metric(f0.regional_frequency,f.regional_frequency,extent,intensity);std::cout<<names[k]<<","<<e<<","<<extent<<","<<intensity<<"\n";if(e&&extent>0&&intensity>0)detected=true;}}return detected?0:1;}
