#include "bit_analyze/adaptive_memory.hpp"
#include "bit_analyze/compiled_encoder.hpp"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#include <fstream>
#endif

namespace {

std::uint64_t current_rss_bytes() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc))) {
        return static_cast<std::uint64_t>(pmc.WorkingSetSize);
    }
    return 0;
#else
    std::ifstream in("/proc/self/statm");
    std::uint64_t pages = 0, resident = 0;
    if (!(in >> pages >> resident)) return 0;
    const long page_size = sysconf(_SC_PAGESIZE);
    return page_size > 0 ? resident * static_cast<std::uint64_t>(page_size) : 0;
#endif
}

std::uint64_t peak_rss_bytes() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc))) {
        return static_cast<std::uint64_t>(pmc.PeakWorkingSetSize);
    }
    return 0;
#else
    rusage u{};
    if (getrusage(RUSAGE_SELF, &u) != 0) return 0;
#ifdef __APPLE__
    return static_cast<std::uint64_t>(u.ru_maxrss);
#else
    return static_cast<std::uint64_t>(u.ru_maxrss) * 1024ULL;
#endif
#endif
}

std::vector<std::uint8_t> make_data(std::size_t bytes) {
    static const std::vector<std::uint8_t> motif{
        'A','B','A','B','C','D','C','D','0','0','0','0',
        'X','Y','Z','X','Y','Z','1','2','3','1','2','3'
    };
    std::vector<std::uint8_t> out(bytes);
    for (std::size_t i = 0; i < bytes; ++i) {
        out[i] = motif[i % motif.size()];
        if ((i % 4096) == 0) out[i] ^= static_cast<std::uint8_t>((i / 4096) & 0xffU);
    }
    return out;
}

double seconds_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

} // namespace

int main() {
    using namespace bit_analyze;

    // Learn on a small deterministic sample so the resource benchmark measures
    // representation at scale rather than repeatedly retraining on each size.
    const auto seed = make_data(256 * 1024);
    AdaptiveMemory memory;
    memory.learn_online(seed, 64, 3, 1.2, 0.001);
    CompiledEncoder compiled(memory);

    std::cout << "input_bytes,rules,trie_nodes,trail_symbols,encode_seconds,decode_seconds,encode_mib_s,decode_mib_s,current_rss_bytes,peak_rss_bytes\n";

    for (const std::size_t bytes : {1ULL << 20, 10ULL << 20, 32ULL << 20}) {
        auto data = make_data(bytes);

        const auto e0 = std::chrono::steady_clock::now();
        auto encoded = compiled.encode(data);
        const double encode_seconds = seconds_since(e0);

        const auto d0 = std::chrono::steady_clock::now();
        auto decoded = memory.decode(encoded.trail);
        const double decode_seconds = seconds_since(d0);
        assert(decoded == data);

        const double mib = static_cast<double>(bytes) / (1024.0 * 1024.0);
        const double enc_rate = encode_seconds > 0.0 ? mib / encode_seconds : 0.0;
        const double dec_rate = decode_seconds > 0.0 ? mib / decode_seconds : 0.0;

        std::cout << bytes << ','
                  << memory.rule_count() << ','
                  << compiled.trie_node_count() << ','
                  << encoded.trail.size() << ','
                  << std::fixed << std::setprecision(6)
                  << encode_seconds << ',' << decode_seconds << ','
                  << enc_rate << ',' << dec_rate << ','
                  << current_rss_bytes() << ',' << peak_rss_bytes() << '\n';
    }

    return 0;
}
