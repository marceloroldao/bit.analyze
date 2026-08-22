#include "bit_analyze/persistence.hpp"

#include <array>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace bit_analyze {

namespace {

constexpr std::array<char, 8> kMagic{{'B','I','T','A','N','L','Z','1'}};

template <typename T>
void write_pod(std::ofstream& out, const T& value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    if (!out) throw std::runtime_error("failed to write snapshot");
}

template <typename T>
T read_pod(std::ifstream& in) {
    T value{};
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!in) throw std::runtime_error("truncated snapshot");
    return value;
}

} // namespace

void save_snapshot(const MemorySnapshot& snapshot, const std::string& path) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("cannot open snapshot for writing");

    out.write(kMagic.data(), static_cast<std::streamsize>(kMagic.size()));
    write_pod(out, snapshot.version);

    const std::uint64_t rule_count = static_cast<std::uint64_t>(snapshot.rules.size());
    const std::uint64_t trail_count = static_cast<std::uint64_t>(snapshot.trails.size());
    write_pod(out, rule_count);
    write_pod(out, trail_count);

    for (const auto& r : snapshot.rules) {
        write_pod(out, static_cast<std::uint64_t>(r.id));
        write_pod(out, static_cast<std::uint64_t>(r.left));
        write_pod(out, static_cast<std::uint64_t>(r.right));
        write_pod(out, static_cast<std::uint64_t>(r.frequency));
    }

    for (const auto& trail : snapshot.trails) {
        write_pod(out, static_cast<std::uint64_t>(trail.size()));
        for (const auto symbol : trail) write_pod(out, static_cast<std::uint64_t>(symbol));
    }
}

MemorySnapshot load_snapshot(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open snapshot for reading");

    std::array<char, 8> magic{};
    in.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (!in || magic != kMagic) throw std::runtime_error("invalid snapshot magic");

    MemorySnapshot snapshot;
    snapshot.version = read_pod<std::uint32_t>(in);
    if (snapshot.version != 1) throw std::runtime_error("unsupported snapshot version");

    const auto rule_count = read_pod<std::uint64_t>(in);
    const auto trail_count = read_pod<std::uint64_t>(in);
    if (rule_count > 100000000ULL || trail_count > 100000000ULL)
        throw std::runtime_error("snapshot count exceeds safety limit");

    snapshot.rules.reserve(static_cast<std::size_t>(rule_count));
    for (std::uint64_t i = 0; i < rule_count; ++i) {
        AdaptiveRule r;
        r.id = read_pod<std::uint64_t>(in);
        r.left = read_pod<std::uint64_t>(in);
        r.right = read_pod<std::uint64_t>(in);
        const auto frequency = read_pod<std::uint64_t>(in);
        if (frequency > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
            throw std::runtime_error("frequency overflow");
        r.frequency = static_cast<std::size_t>(frequency);
        snapshot.rules.push_back(r);
    }

    snapshot.trails.reserve(static_cast<std::size_t>(trail_count));
    for (std::uint64_t t = 0; t < trail_count; ++t) {
        const auto n = read_pod<std::uint64_t>(in);
        if (n > 1000000000ULL) throw std::runtime_error("trail length exceeds safety limit");
        std::vector<SymbolId> trail;
        trail.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t i = 0; i < n; ++i) trail.push_back(read_pod<std::uint64_t>(in));
        snapshot.trails.push_back(std::move(trail));
    }

    return snapshot;
}

} // namespace bit_analyze
