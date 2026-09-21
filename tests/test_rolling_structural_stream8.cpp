#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include "bit_analyze/hierarchical_memory.hpp"
#include "bit_analyze/structural_stream.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
using namespace bit_analyze;
namespace {
RollingStructuralFingerprintConfig config(){RollingStructuralFingerprintConfig c;c.bounded.frequency_buckets=256;c.bounded.transition_buckets=256;c.bounded.regions=8;c.bounded.hash_seed=0xB17A2026ULL;c.region_span_bytes=1024;return c;}
std::vector<std::uint8_t> input(){std::vector<std::uint8_t>x(65536);for(std::size_t i=0;i<x.size();++i)x[i]=static_cast<std::uint8_t>(((i%251)*17+(i/251)%239)&255);return x;}
RollingStructuralFingerprint8 run(const std::vector<std::uint8_t>&x,std::size_t chunk){HierarchicalMemory m;StructuralExtractor extractor(m,2);StructuralStream stream(extractor,128,128);RollingStructuralFingerprintAccumulator8 acc(config());RollingStructuralStream8 pipeline(stream,acc);for(std::size_t p=0;p<x.size();p+=chunk){const auto n=std::min(chunk,x.size()-p);pipeline.push(x.data()+p,n,"e2e");}pipeline.flush("e2e");return pipeline.fingerprint();}
bool equal(const RollingStructuralFingerprint8&a,const RollingStructuralFingerprint8&b){return a.event_count==b.event_count&&a.renormalizations==b.renormalizations&&a.region_reuses==b.region_reuses&&a.latest_epoch==b.latest_epoch&&a.frequency==b.frequency&&a.regional_frequency==b.regional_frequency&&a.transitions==b.transitions&&a.region_epochs==b.region_epochs;}
}
int main(){const auto x=input();const auto a=run(x,17),b=run(x,257),c=run(x,4096),d=run(x,x.size());assert(equal(a,b));assert(equal(a,c));assert(equal(a,d));assert(a.event_count>0);assert(a.storage_bytes()==256u+8u*256u+256u+8u*sizeof(std::uint64_t));return 0;}
