#include "bit_analyze/structural_stream.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <vector>

using namespace bit_analyze;

namespace {
struct Fingerprint {
    std::map<SymbolId,std::size_t> frequency;
    std::vector<SymbolId> ordered;
    std::size_t events{};
};

Fingerprint analyze(const std::vector<std::uint8_t>& data,std::size_t chunk) {
    HierarchicalMemory memory;
    StructuralExtractor extractor(memory,2);
    StructuralStream stream(extractor,128,128);
    Fingerprint fp;
    std::size_t pos=0;
    auto collect=[&](const std::vector<StructuralEvent>& events){
        for(const auto& e:events){
            ++fp.events;
            for(auto id:e.relation_ids){++fp.frequency[id]; fp.ordered.push_back(id);}
        }
    };
    while(pos<data.size()) {
        auto n=std::min(chunk,data.size()-pos);
        collect(stream.push(data.data()+pos,n,"blind")); pos+=n;
    }
    collect(stream.flush("blind"));
    return fp;
}

double weighted_jaccard(const Fingerprint&a,const Fingerprint&b){
    std::map<SymbolId,std::pair<std::size_t,std::size_t>> all;
    for(auto [k,v]:a.frequency) all[k].first=v;
    for(auto [k,v]:b.frequency) all[k].second=v;
    double mn=0,mx=0; for(auto [k,v]:all){mn+=std::min(v.first,v.second);mx+=std::max(v.first,v.second);} return mx?mn/mx:1.0;
}

double ordered_similarity(const Fingerprint&a,const Fingerprint&b){
    const auto n=std::max(a.ordered.size(),b.ordered.size()); if(!n)return 1.0;
    const auto m=std::min(a.ordered.size(),b.ordered.size()); std::size_t same=0;
    for(std::size_t i=0;i<m;++i) if(a.ordered[i]==b.ordered[i]) ++same;
    return double(same)/double(n);
}

std::vector<std::uint8_t> base_data(){
    std::vector<std::uint8_t> v(8192);
    for(std::size_t i=0;i<v.size();++i) v[i]=static_cast<std::uint8_t>(((i%97)*13 + (i/97)%31) & 0xff);
    return v;
}
std::vector<std::uint8_t> mutate(const std::vector<std::uint8_t>& in,double fraction){
    auto v=in; std::mt19937 rng(0xB17A2026); std::vector<std::size_t> idx(v.size());
    for(std::size_t i=0;i<idx.size();++i)idx[i]=i; std::shuffle(idx.begin(),idx.end(),rng);
    auto count=static_cast<std::size_t>(std::ceil(v.size()*fraction)); count=std::min(count,v.size());
    for(std::size_t i=0;i<count;++i) v[idx[i]]^=static_cast<std::uint8_t>(1u << (i%8));
    return v;
}
}

int main(){
    const auto base=base_data(); const auto reference=analyze(base,17);
    std::cout<<"structural_mutation_gradient_v0\n";
    std::cout<<"mutation_fraction,mutated_bytes,weighted_jaccard,ordered_similarity,chunk_invariant\n";
    std::cout<<std::fixed<<std::setprecision(6);
    for(double f:{0.0,1.0/double(base.size()),0.001,0.01,0.05,0.10,0.25,0.50}){
        auto data=mutate(base,f); auto a=analyze(data,17); auto b=analyze(data,257);
        bool invariant=a.frequency==b.frequency && a.ordered==b.ordered && a.events==b.events;
        auto changed=static_cast<std::size_t>(std::ceil(base.size()*f)); if(f==0.0)changed=0;
        std::cout<<f<<','<<changed<<','<<weighted_jaccard(reference,a)<<','<<ordered_similarity(reference,a)<<','<<(invariant?1:0)<<'\n';
    }
}
