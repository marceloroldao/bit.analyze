#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include "bit_analyze/hierarchical_memory.hpp"
#include "bit_analyze/structural_stream.hpp"
#include <cstdint>
#include <iostream>
#include <vector>
using namespace bit_analyze;
static std::vector<std::uint8_t> block(std::size_t n,bool diverse){std::vector<std::uint8_t>x(n);for(std::size_t i=0;i<n;++i)x[i]=diverse?static_cast<std::uint8_t>((i*73+(i/31)*19)&255):static_cast<std::uint8_t>((i%16)<8?0xAA:0x55);return x;}
static RollingStructuralFingerprint8 run(const std::vector<std::uint8_t>&x){RollingStructuralFingerprintConfig c;c.bounded.frequency_buckets=256;c.bounded.transition_buckets=256;c.bounded.regions=8;c.region_span_bytes=4096;HierarchicalMemory m;StructuralExtractor e(m,2);StructuralStream s(e,128,128);RollingStructuralFingerprintAccumulator8 a(c);RollingStructuralStream8 p(s,a);p.push(x,"checkpoint");p.flush("checkpoint");return p.fingerprint();}
static std::size_t diff(const std::vector<std::uint8_t>&a,const std::vector<std::uint8_t>&b){std::size_t n=0;for(std::size_t i=0;i<a.size();++i)n+=a[i]!=b[i];return n;}
int main(){auto A=block(32768,false),B=block(32768,true),AA=A,AB=A;AA.insert(AA.end(),A.begin(),A.end());AB.insert(AB.end(),B.begin(),B.end());auto base=run(A),same=run(AA),changed=run(AB);auto d_same=diff(base.regional_frequency,same.regional_frequency);auto d_change=diff(base.regional_frequency,changed.regional_frequency);std::cout<<"structural_checkpoint_change_v0\ncase,regional_bucket_changes,global_frequency_changed,transitions_changed\n";std::cout<<"A_to_A,"<<d_same<<","<<(base.frequency!=same.frequency)<<","<<(base.transitions!=same.transitions)<<"\n";std::cout<<"A_to_B,"<<d_change<<","<<(base.frequency!=changed.frequency)<<","<<(base.transitions!=changed.transitions)<<"\n";if(d_change<=d_same)return 1;if(changed.frequency==base.frequency||changed.transitions==base.transitions)return 1;return 0;}
