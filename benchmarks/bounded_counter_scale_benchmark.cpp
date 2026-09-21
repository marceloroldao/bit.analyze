#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>
using namespace bit_analyze;
namespace {
std::vector<std::uint8_t> data(std::size_t n){std::vector<std::uint8_t>v(n);for(std::size_t i=0;i<n;++i)v[i]=static_cast<std::uint8_t>(((i%97)*13+(i/97)%31)&0xff);return v;}
std::vector<StructuralEvent> events_for(const std::vector<std::uint8_t>&d){HierarchicalMemory m;StructuralExtractor x(m,2);StructuralStream s(x,128,128);std::vector<StructuralEvent>o;for(std::size_t p=0;p<d.size();){auto n=std::min<std::size_t>(4096,d.size()-p);auto e=s.push(d.data()+p,n,"scale");o.insert(o.end(),e.begin(),e.end());p+=n;}auto e=s.flush("scale");o.insert(o.end(),e.begin(),e.end());return o;}
struct Stats{std::size_t saturated{};std::uint64_t maximum{};};
Stats stats(const BoundedStructuralFingerprint&f,std::uint64_t limit){Stats s;auto scan=[&](const std::vector<std::uint64_t>&v){for(auto x:v){s.maximum=std::max(s.maximum,x);if(x>limit)++s.saturated;}};scan(f.frequency);scan(f.regional_frequency);scan(f.transitions);return s;}
}
int main(){BoundedStructuralFingerprintConfig c;c.frequency_buckets=4096;c.transition_buckets=4096;c.regions=8;std::cout<<"bounded_counter_scale_4096\nbytes,events,relations,max_counter,saturated_8,saturated_16,saturated_32,logical_8_bytes,logical_16_bytes\n";for(std::size_t n:{8192u,65536u,1048576u,16777216u}){auto d=data(n);auto e=events_for(d);auto f=make_bounded_structural_fingerprint(e,n,c);std::size_t rel=0;for(const auto&x:e)rel+=x.relation_ids.size();auto s8=stats(f,255),s16=stats(f,65535),s32=stats(f,std::numeric_limits<std::uint32_t>::max());auto counters=c.frequency_buckets+c.regions*c.frequency_buckets+c.transition_buckets;std::cout<<n<<','<<e.size()<<','<<rel<<','<<s8.maximum<<','<<s8.saturated<<','<<s16.saturated<<','<<s32.saturated<<','<<counters<<','<<counters*2<<'\n';}}
