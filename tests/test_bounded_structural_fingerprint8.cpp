#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include <cassert>
#include <stdexcept>
#include <vector>
using namespace bit_analyze;
int main(){
 BoundedStructuralFingerprintConfig c;c.frequency_buckets=64;c.transition_buckets=64;c.regions=8;c.hash_seed=0xB17A2026ULL;
 std::vector<StructuralEvent> events;events.reserve(1024);
 for(std::uint64_t i=0;i<1024;++i){StructuralEvent e;e.byte_offset=i;e.byte_length=1;e.relation_ids={static_cast<SymbolId>(256+i%7),static_cast<SymbolId>(300+i%3)};events.push_back(e);}
 BoundedStructuralFingerprintAccumulator8 a(1024,c),b(1024,c);
 for(const auto&e:events){a.observe(e);b.observe(e);}
 const auto&x=a.fingerprint();const auto&y=b.fingerprint();
 assert(x.version==2);assert(x.event_count==events.size());assert(x.storage_bytes()==64+8*64+64);assert(x.renormalizations>0);
 assert(x.frequency==y.frequency);assert(x.regional_frequency==y.regional_frequency);assert(x.transitions==y.transitions);assert(x.renormalizations==y.renormalizations);
 auto ex=expand_bounded_structural_fingerprint(x),ey=expand_bounded_structural_fingerprint(y);assert(compare_bounded_structural_fingerprints(ex,ey).combined==1.0);
 BoundedStructuralFingerprintAccumulator8 split(1024,c);for(std::size_t p=0;p<events.size();p+=17)for(std::size_t i=p;i<events.size()&&i<p+17;++i)split.observe(events[i]);
 const auto&z=split.fingerprint();assert(z.frequency==x.frequency);assert(z.regional_frequency==x.regional_frequency);assert(z.transitions==x.transitions);assert(z.renormalizations==x.renormalizations);
 bool threw=false;try{auto bad=c;bad.frequency_buckets=0;BoundedStructuralFingerprintAccumulator8 invalid(1,bad);}catch(const std::invalid_argument&){threw=true;}assert(threw);
 return 0;
}
