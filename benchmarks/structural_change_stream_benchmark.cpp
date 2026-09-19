#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include "bit_analyze/hierarchical_memory.hpp"
#include "bit_analyze/structural_stream.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>
using namespace bit_analyze;
static std::vector<std::uint8_t> block(std::size_t n,bool b){std::vector<std::uint8_t>x(n);for(std::size_t i=0;i<n;++i)x[i]=b?static_cast<std::uint8_t>((i*73+(i/31)*19)&255):static_cast<std::uint8_t>((i%16)<8?0xAA:0x55);return x;}
static std::uint64_t regional_sum(const RollingStructuralFingerprint8&f,std::size_t r){auto n=f.config.bounded.frequency_buckets;return std::accumulate(f.regional_frequency.begin()+r*n,f.regional_frequency.begin()+(r+1)*n,std::uint64_t{0});}
int main(){RollingStructuralFingerprintConfig cfg;cfg.bounded.frequency_buckets=256;cfg.bounded.transition_buckets=256;cfg.bounded.regions=8;cfg.region_span_bytes=4096;HierarchicalMemory m;StructuralExtractor ex(m,2);StructuralStream ss(ex,128,128);RollingStructuralFingerprintAccumulator8 acc(cfg);RollingStructuralStream8 p(ss,acc);auto a=block(32768,false),b=block(32768,true);p.push(a,"change");auto before=p.fingerprint();p.push(b,"change");p.flush("change");auto after=p.fingerprint();std::cout<<"structural_change_stream_v0\n"<<"phase,event_count,latest_epoch,region_reuses,renormalizations,active_region_sum\n";auto rb=static_cast<std::size_t>(before.latest_epoch%cfg.bounded.regions),ra=static_cast<std::size_t>(after.latest_epoch%cfg.bounded.regions);std::cout<<"A,"<<before.event_count<<","<<before.latest_epoch<<","<<before.region_reuses<<","<<before.renormalizations<<","<<regional_sum(before,rb)<<"\n";std::cout<<"B,"<<after.event_count<<","<<after.latest_epoch<<","<<after.region_reuses<<","<<after.renormalizations<<","<<regional_sum(after,ra)<<"\n";if(after.latest_epoch<=before.latest_epoch||after.region_reuses<=before.region_reuses||after.frequency==before.frequency||after.regional_frequency==before.regional_frequency||after.transitions==before.transitions)return 1;return 0;}
