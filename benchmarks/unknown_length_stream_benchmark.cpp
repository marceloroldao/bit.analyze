#include "bit_analyze/structural_stream.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>
using namespace bit_analyze;
namespace {
struct RollingRegions {
 std::size_t buckets=4096, regions=8; std::uint64_t span=65536, seed=0xB17A2026ULL;
 std::vector<std::uint64_t> counts=std::vector<std::uint64_t>(buckets*regions,0);
 std::vector<std::uint64_t> tags=std::vector<std::uint64_t>(regions,std::numeric_limits<std::uint64_t>::max());
 std::size_t reuses=0; std::uint64_t max_epoch=0;
 static std::uint64_t mix(std::uint64_t x){x+=0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);}
 void observe(const StructuralEvent&e){auto epoch=e.byte_offset/span, region=static_cast<std::size_t>(epoch%regions);max_epoch=std::max(max_epoch,epoch);if(tags[region]!=epoch){if(tags[region]!=std::numeric_limits<std::uint64_t>::max())++reuses;std::fill_n(counts.begin()+region*buckets,buckets,0);tags[region]=epoch;}for(auto id:e.relation_ids)++counts[region*buckets+static_cast<std::size_t>(mix(static_cast<std::uint64_t>(id)^seed)%buckets)];}
 bool operator==(const RollingRegions&o)const{return counts==o.counts&&tags==o.tags&&reuses==o.reuses&&max_epoch==o.max_epoch;}
 std::size_t storage_bytes()const{return counts.size()*sizeof(std::uint64_t)+tags.size()*sizeof(std::uint64_t);}
};
std::vector<std::uint8_t> make_data(std::size_t n,bool repetitive){std::vector<std::uint8_t>d(n);for(std::size_t i=0;i<n;++i)d[i]=repetitive?static_cast<std::uint8_t>(i%4):static_cast<std::uint8_t>(((i%251)*17+(i/251)%239)&255);return d;}
RollingRegions run(const std::vector<std::uint8_t>&d,std::size_t chunk){HierarchicalMemory m;StructuralExtractor x(m,2);StructuralStream s(x,128,128);RollingRegions r;for(std::size_t p=0;p<d.size();){auto n=std::min(chunk,d.size()-p);for(const auto&e:s.push(d.data()+p,n,"unknown"))r.observe(e);p+=n;}for(const auto&e:s.flush("unknown"))r.observe(e);return r;}
}
int main(){std::cout<<"unknown_length_rolling_regions_v0\ninput_kind,input_bytes,chunk,epochs_seen,region_reuses,chunk_invariant,storage_bytes\n";for(bool rep:{false,true})for(std::size_t n:{65536u,1048576u,16777216u}){auto d=make_data(n,rep);auto a=run(d,17),b=run(d,257),c=run(d,4096);bool invariant=(a==b&&a==c);std::cout<<(rep?"repetitive":"diverse")<<','<<n<<",17,"<<(a.max_epoch+1)<<','<<a.reuses<<','<<(invariant?1:0)<<','<<a.storage_bytes()<<'\n';if(!invariant)return 2;}return 0;}
