#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>
using namespace bit_analyze;
namespace {
StructuralEvent ev(std::uint64_t off,std::initializer_list<SymbolId> ids){StructuralEvent e;e.byte_offset=off;e.byte_length=1;e.relation_ids.assign(ids);return e;}
bool equal(const RollingStructuralFingerprint8&a,const RollingStructuralFingerprint8&b){return a.event_count==b.event_count&&a.renormalizations==b.renormalizations&&a.region_reuses==b.region_reuses&&a.latest_epoch==b.latest_epoch&&a.frequency==b.frequency&&a.regional_frequency==b.regional_frequency&&a.transitions==b.transitions&&a.region_epochs==b.region_epochs;}
RollingStructuralFingerprintConfig cfg(){RollingStructuralFingerprintConfig c;c.bounded.frequency_buckets=64;c.bounded.transition_buckets=64;c.bounded.regions=4;c.bounded.hash_seed=0xB17A2026ULL;c.region_span_bytes=32;return c;}
}
int main(){
 auto c=cfg();
 // Determinism and fragmentation-equivalent delivery.
 std::vector<StructuralEvent> events;for(std::uint64_t i=0;i<1024;++i)events.push_back(ev(i,{static_cast<SymbolId>(256+(i%7)),static_cast<SymbolId>(300+(i%3))}));
 RollingStructuralFingerprintAccumulator8 a(c),b(c),grouped(c);
 for(const auto&e:events){a.observe(e);b.observe(e);}
 for(std::size_t p=0;p<events.size();p+=17)for(std::size_t i=p;i<std::min(events.size(),p+17);++i)grouped.observe(events[i]);
 assert(equal(a.fingerprint(),b.fingerprint()));assert(equal(a.fingerprint(),grouped.fingerprint()));
 assert(a.fingerprint().renormalizations>0);
 // Fixed logical storage independent of duration.
 const auto expected=64u+4u*64u+64u+4u*sizeof(std::uint64_t);
 assert(a.fingerprint().storage_bytes()==expected);
 RollingStructuralFingerprintAccumulator8 short_run(c);for(std::size_t i=0;i<8;++i)short_run.observe(events[i]);
 assert(short_run.fingerprint().storage_bytes()==expected);
 // Region recycling clears stale regional state and records reuse.
 RollingStructuralFingerprintAccumulator8 recycle(c);recycle.observe(ev(0,{256}));auto before=recycle.fingerprint().regional_frequency;recycle.observe(ev(4*32,{257}));
 assert(recycle.fingerprint().region_reuses==1);assert(recycle.fingerprint().region_epochs[0]==4);
 bool stale_present=false;for(std::size_t i=0;i<64;++i)if(recycle.fingerprint().regional_frequency[i]&&before[i])stale_present=true;
 // Collision can share a bucket, so validate total count rather than bucket identity.
 std::size_t sum=0;for(std::size_t i=0;i<64;++i)sum+=recycle.fingerprint().regional_frequency[i];assert(sum==1);(void)stale_present;
 // Reject stale/out-of-order offsets.
 bool threw=false;try{RollingStructuralFingerprintAccumulator8 order(c);order.observe(ev(100,{256}));order.observe(ev(99,{257}));}catch(const std::invalid_argument&){threw=true;}assert(threw);
 // Invalid dimensions/span.
 threw=false;try{auto bad=c;bad.region_span_bytes=0;RollingStructuralFingerprintAccumulator8 x(bad);(void)x;}catch(const std::invalid_argument&){threw=true;}assert(threw);
 assert(a.fingerprint().version==3);
 return 0;
}
