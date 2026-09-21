#include "bit_analyze/structural_stream.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace bit_analyze;

namespace {
std::vector<std::uint8_t> bytes(const std::string& s) { return {s.begin(), s.end()}; }
std::vector<std::uint8_t> pseudo_image() { std::vector<std::uint8_t> v; for(int y=0;y<32;++y) for(int x=0;x<32;++x) v.push_back(static_cast<std::uint8_t>((x/4+y/4)%2?220:30)); return v; }
std::vector<std::uint8_t> pseudo_audio() { std::vector<std::uint8_t> v(2048); for(std::size_t i=0;i<v.size();++i) v[i]=static_cast<std::uint8_t>(128+((i%64)<32?70:-70)); return v; }
std::vector<std::uint8_t> make_random_bytes() { std::mt19937 rng(1234567); std::vector<std::uint8_t> v(2048); for(auto& b:v) b=static_cast<std::uint8_t>(rng()&0xff); return v; }
std::vector<std::uint8_t> repetitive() { std::vector<std::uint8_t> v; const std::array<std::uint8_t,8> p{{1,2,3,5,8,13,21,34}}; for(int i=0;i<256;++i) v.insert(v.end(),p.begin(),p.end()); return v; }
std::vector<std::uint8_t> pseudo_binary() { std::vector<std::uint8_t> v; for(int r=0;r<128;++r) for(int i=0;i<16;++i) v.push_back(static_cast<std::uint8_t>((i*17+r)%256)); return v; }
struct Fingerprint { std::set<SymbolId> ids; std::size_t events{}; };
Fingerprint analyze(const std::vector<std::uint8_t>& data,std::size_t chunk){ HierarchicalMemory memory; StructuralExtractor ex(memory,2); StructuralStream stream(ex,128,128); Fingerprint fp; std::size_t pos=0; while(pos<data.size()){auto n=std::min(chunk,data.size()-pos); auto ev=stream.push(data.data()+pos,n,"blind"); for(const auto& e:ev){fp.ids.insert(e.relation_ids.begin(),e.relation_ids.end());++fp.events;} pos+=n;} for(const auto& e:stream.flush("blind")){fp.ids.insert(e.relation_ids.begin(),e.relation_ids.end());++fp.events;} return fp; }
double jaccard(const Fingerprint&a,const Fingerprint&b){std::size_t inter=0;for(auto x:a.ids)if(b.ids.count(x))++inter;auto uni=a.ids.size()+b.ids.size()-inter;return uni?double(inter)/double(uni):1.0;}
}
int main(){
 std::vector<std::pair<std::string,std::vector<std::uint8_t>>> corpus{
 {"sample_a",bytes("The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog.")},
 {"sample_b",bytes("<html><body><div>alpha</div><div>alpha</div><p>beta</p></body></html>")},
 {"sample_c",bytes("{\"alpha\":[1,2,3,1,2,3],\"beta\":{\"x\":7,\"x2\":7}}")},
 {"sample_d",pseudo_image()},{"sample_e",pseudo_audio()},{"sample_f",pseudo_binary()},{"sample_g",make_random_bytes()},{"sample_h",repetitive()}};
 std::cout<<"format_blind_structural_benchmark_v1\nname,bytes,relations,events,chunk_invariant\n"; std::vector<Fingerprint> fps;
 for(const auto&s:corpus){auto a=analyze(s.second,17),b=analyze(s.second,257);bool inv=a.ids==b.ids&&a.events==b.events;std::cout<<s.first<<','<<s.second.size()<<','<<a.ids.size()<<','<<a.events<<','<<(inv?1:0)<<'\n';fps.push_back(std::move(a));}
 auto modified=corpus[0].second;if(!modified.empty())modified[modified.size()/2]^=1;auto fm=analyze(modified,17);std::cout<<std::fixed<<std::setprecision(6);
 std::cout<<"repeat_similarity,"<<jaccard(fps[0],analyze(corpus[0].second,31))<<'\n';
 std::cout<<"small_change_similarity,"<<jaccard(fps[0],fm)<<'\n';
 std::cout<<"text_vs_random_similarity,"<<jaccard(fps[0],fps[6])<<'\n';
 std::cout<<"repetitive_vs_random_similarity,"<<jaccard(fps[7],fps[6])<<'\n';return 0;
}
