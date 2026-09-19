#include "bit_analyze/structural_fingerprint.hpp"
#include <algorithm>
#include <map>
namespace bit_analyze {
namespace {
template<class K> double weighted_jaccard(const std::map<K,std::size_t>& a,const std::map<K,std::size_t>& b){
 std::map<K,std::pair<std::size_t,std::size_t>> all; for(const auto& x:a)all[x.first].first=x.second; for(const auto& x:b)all[x.first].second=x.second;
 double mn=0,mx=0; for(const auto& x:all){mn+=std::min(x.second.first,x.second.second);mx+=std::max(x.second.first,x.second.second);} return mx?mn/mx:1.0;
}
}
StructuralFingerprint make_structural_fingerprint(const std::vector<StructuralEvent>& events,std::uint64_t total_bytes,std::size_t regions){
 StructuralFingerprint fp; fp.total_bytes=total_bytes; fp.event_count=events.size(); fp.regional_frequency.resize(std::max<std::size_t>(1,regions));
 bool have_prev=false; SymbolId prev{};
 for(const auto& e:events){
  std::size_t r=0; if(total_bytes>0) r=std::min(fp.regional_frequency.size()-1,static_cast<std::size_t>((e.byte_offset*fp.regional_frequency.size())/total_bytes));
  for(auto id:e.relation_ids){++fp.relation_frequency[id];++fp.regional_frequency[r][id];if(have_prev)++fp.transitions[{prev,id}];prev=id;have_prev=true;}
 }
 return fp;
}
StructuralSimilarity compare_structural_fingerprints(const StructuralFingerprint&a,const StructuralFingerprint&b){
 StructuralSimilarity s; s.frequency=weighted_jaccard(a.relation_frequency,b.relation_frequency); s.transitions=weighted_jaccard(a.transitions,b.transitions);
 const auto n=std::max(a.regional_frequency.size(),b.regional_frequency.size()); double sum=0;
 for(std::size_t i=0;i<n;++i){static const std::map<SymbolId,std::size_t> empty;const auto&x=i<a.regional_frequency.size()?a.regional_frequency[i]:empty;const auto&y=i<b.regional_frequency.size()?b.regional_frequency[i]:empty;sum+=weighted_jaccard(x,y);}
 s.regional=n?sum/n:1.0; s.combined=0.40*s.frequency+0.35*s.regional+0.25*s.transitions; return s;
}
} // namespace bit_analyze
