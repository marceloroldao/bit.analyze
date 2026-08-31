#include "bit_analyze/adaptive_memory.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {
std::vector<std::uint8_t> read_all(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::size_t fixed8_unique_bytes(const std::vector<std::uint8_t>& d) {
    std::unordered_set<std::string> u;
    for (std::size_t i=0;i<d.size();i+=8) {
        const auto n=std::min<std::size_t>(8,d.size()-i);
        u.emplace(reinterpret_cast<const char*>(d.data()+i), n);
    }
    std::size_t total=0; for (const auto& s:u) total+=s.size(); return total;
}

std::size_t bigram_unique_bytes(const std::vector<std::uint8_t>& d) {
    std::array<bool,65536> seen{}; std::size_t n=0;
    for (std::size_t i=0;i+1<d.size();++i) {
        const auto k=(static_cast<unsigned>(d[i])<<8)|d[i+1];
        if(!seen[k]) { seen[k]=true; ++n; }
    }
    return n*2;
}

std::size_t rle_size(const std::vector<std::uint8_t>& d) {
    if(d.empty()) return 0; std::size_t out=0;
    for(std::size_t i=0;i<d.size();) {
        std::size_t j=i+1; while(j<d.size() && d[j]==d[i] && j-i<255) ++j;
        out += 2; i=j;
    }
    return out;
}

// Small deterministic LZ77-style baseline. Token model: literal=2 bytes,
// match=4 bytes. This is intentionally simple and not a zstd/gzip substitute.
std::size_t lz77_size(const std::vector<std::uint8_t>& d) {
    constexpr std::size_t W=4096, MIN=4, MAX=255;
    std::size_t out=0;
    for(std::size_t i=0;i<d.size();) {
        std::size_t best=0;
        const auto begin=i>W?i-W:0;
        for(std::size_t p=begin;p<i;++p) {
            std::size_t n=0;
            while(n<MAX && i+n<d.size() && p+n<i && d[p+n]==d[i+n]) ++n;
            if(n>best) best=n;
        }
        if(best>=MIN) { out+=4; i+=best; }
        else { out+=2; ++i; }
    }
    return out;
}
}

int main(int argc, char** argv) {
    if(argc<2) { std::cerr << "usage: bit_analyze_same_corpus_baselines FILE [FILE ...]\n"; return 2; }
    bit_analyze::AdaptiveMemory memory;
    std::vector<std::vector<std::uint8_t>> corpus;
    for(int i=1;i<argc;++i) { auto d=read_all(argv[i]); if(!d.empty()) corpus.push_back(std::move(d)); }
    if(corpus.empty()) return 3;

    memory.train(corpus, 256, 3, 1.5, 0.001);
    std::cout << "file,input,adaptive_trail_bytes,fixed8_unique_bytes,bigram_unique_bytes,rle_bytes,lz77_bytes\n";
    double a=0,f=0,b=0,r=0,l=0;
    for(int i=1;i<argc;++i) {
        auto d=read_all(argv[i]); if(d.empty()) continue;
        auto e=memory.encode(d); if(memory.decode(e.trail)!=d) return 4;
        const auto ab=e.trail.size()*sizeof(bit_analyze::SymbolId);
        const auto fb=fixed8_unique_bytes(d), bb=bigram_unique_bytes(d), rb=rle_size(d), lb=lz77_size(d);
        std::cout<<argv[i]<<','<<d.size()<<','<<ab<<','<<fb<<','<<bb<<','<<rb<<','<<lb<<'\n';
        a+=double(ab)/d.size(); f+=double(fb)/d.size(); b+=double(bb)/d.size(); r+=double(rb)/d.size(); l+=double(lb)/d.size();
    }
    const double n=double(corpus.size());
    std::cout<<std::fixed<<std::setprecision(6)
             <<"mean_ratio,adaptive="<<a/n<<",fixed8="<<f/n<<",bigram="<<b/n<<",rle="<<r/n<<",lz77="<<l/n<<'\n';
}
