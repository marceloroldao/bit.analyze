#include "bit_analyze/bounded_structural_fingerprint.hpp"
#include <cassert>
using namespace bit_analyze;
int main(){
 BoundedStructuralFingerprintConfig c;c.frequency_buckets=64;c.transition_buckets=64;c.regions=8;
 StructuralEvent e;e.byte_offset=0;e.relation_ids={1,2,1,2};
 BoundedStructuralFingerprintAccumulator8 a(1024,c),b(1024,c);
 for(int i=0;i<1000;++i){e.byte_offset=static_cast<std::uint64_t>(i%1024);a.observe(e);b.observe(e);}
 const auto&x=a.fingerprint();const auto&y=b.fingerprint();
 assert(x.frequency==y.frequency);assert(x.regional_frequency==y.regional_frequency);assert(x.transitions==y.transitions);
 assert(x.renormalizations>0);assert(x.storage_bytes()==(64+8*64+64));
 auto ex=expand_bounded_structural_fingerprint(x),ey=expand_bounded_structural_fingerprint(y);
 assert(compare_bounded_structural_fingerprints(ex,ey).combined==1.0);
 BoundedStructuralFingerprintAccumulator8 z(1024,c);for(int i=0;i<1000;++i){e.byte_offset=static_cast<std::uint64_t>(i%1024);z.observe(e);}
 assert(z.fingerprint().frequency==x.frequency);
 return 0;
}
