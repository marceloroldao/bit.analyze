#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include "bit_analyze/hierarchical_memory.hpp"
#include "bit_analyze/structural_stream.hpp"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
using namespace bit_analyze;
static std::vector<std::uint8_t> block(std::size_t n,int phase,bool anomaly){std::vector<std::uint8_t>x(n);for(std::size_t i=0;i<n;++i){auto j=i+phase;if(anomaly)x[i]=static_cast<std::uint8_t>((j*73+(j/31)*19)&255);else x[i]=static_cast<std::uint8_t>((j%16)<8?0xAA:0x55);}return x;}
static RollingStructuralFingerprint8 run(const std::vector<std::uint8_t>&x){RollingStructuralFingerprintConfig c;c.bounded.frequency_buckets=256;c.bounded.transition_buckets=256;c.bounded.regions=8;c.region_span_bytes=4096;HierarchicalMemory m;StructuralExtractor e(m,2);StructuralStream s(e,128,128);RollingStructuralFingerprintAccumulator8 a(c);RollingStructuralStream8 p(s,a);p.push(x,"baseline");p.flush("baseline");return p.fingerprint();}
static void metric(const std::vector<std::uint8_t>&a,const std::vector<std::uint8_t>&b,double&extent,double&intensity){std::size_t changed=0;double mag=0;for(std::size_t i=0;i<a.size();++i)if(a[i]!=b[i]){++changed;mag+=std::abs(int(a[i])-int(b[i]))/255.0;}extent=a.empty()?0:double(changed)/a.size();intensity=changed?mag/changed:0;}
int main(){auto ref=run(block(65536,0,false));double be=0,bi=0;for(int p=1;p<=7;++p){auto f=run(block(65536,p,false));double e,i;metric(ref.regional_frequency,f.regional_frequency,e,i);be+=e;bi+=i;}be/=7;bi/=7;auto abnormal=run(block(65536,0,true));double ae,ai;metric(ref.regional_frequency,abnormal.regional_frequency,ae,ai);std::cout<<"structural_baseline_v0\nbaseline_extent,baseline_intensity,anomaly_extent,anomaly_intensity\n"<<be<<","<<bi<<","<<ae<<","<<ai<<"\n";return (ae>be||ai>bi)?0:1;}
