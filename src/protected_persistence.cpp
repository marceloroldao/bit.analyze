#include "bit_analyze/protected_persistence.hpp"

#include <array>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace bit_analyze {
namespace {

constexpr std::array<char, 8> kMagic{{'B','I','T','P','R','O','T','2'}};
constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

template <typename T>
void write_pod(std::ofstream& out, const T& value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    if (!out) throw std::runtime_error("failed to write protected snapshot");
}

template <typename T>
T read_pod(std::ifstream& in) {
    T value{};
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!in) throw std::runtime_error("truncated protected snapshot");
    return value;
}

void write_bytes(std::ofstream& out, const std::array<std::uint8_t, 8>& a) {
    out.write(reinterpret_cast<const char*>(a.data()), static_cast<std::streamsize>(a.size()));
    if (!out) throw std::runtime_error("failed to write parity bytes");
}

std::array<std::uint8_t, 8> read_bytes(std::ifstream& in) {
    std::array<std::uint8_t, 8> a{};
    in.read(reinterpret_cast<char*>(a.data()), static_cast<std::streamsize>(a.size()));
    if (!in) throw std::runtime_error("truncated parity bytes");
    return a;
}

std::uint64_t checksum_prefix(const std::string& path, std::uint64_t bytes) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open protected snapshot for checksum");
    std::uint64_t h = kFnvOffset;
    std::array<char, 8192> buf{};
    std::uint64_t remaining = bytes;
    while (remaining > 0) {
        const auto want = static_cast<std::streamsize>(
            remaining < buf.size() ? remaining : static_cast<std::uint64_t>(buf.size()));
        in.read(buf.data(), want);
        const auto got = in.gcount();
        if (got <= 0) throw std::runtime_error("truncated protected snapshot during checksum");
        for (std::streamsize i = 0; i < got; ++i) {
            h ^= static_cast<std::uint8_t>(buf[static_cast<std::size_t>(i)]);
            h *= kFnvPrime;
        }
        remaining -= static_cast<std::uint64_t>(got);
    }
    return h;
}

std::uint64_t file_size(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) throw std::runtime_error("cannot stat protected snapshot");
    const auto end = in.tellg();
    if (end < 0) throw std::runtime_error("cannot determine protected snapshot size");
    return static_cast<std::uint64_t>(end);
}

void verify_file_checksum(const std::string& path) {
    const auto size = file_size(path);
    if (size < sizeof(std::uint64_t)) throw std::runtime_error("protected snapshot missing checksum");
    const auto payload_size = size - sizeof(std::uint64_t);

    std::ifstream in(path, std::ios::binary);
    in.seekg(static_cast<std::streamoff>(payload_size));
    const auto stored = read_pod<std::uint64_t>(in);
    const auto actual = checksum_prefix(path, payload_size);
    if (stored != actual) throw std::runtime_error("protected snapshot checksum mismatch");
}

std::uint8_t profile_byte(ProtectionProfile p) {
    return static_cast<std::uint8_t>(p);
}

ProtectionProfile read_profile(std::uint8_t v) {
    if (v == 2) return ProtectionProfile::Light;
    if (v == 4) return ProtectionProfile::Medium;
    if (v == 6) return ProtectionProfile::Strong;
    throw std::runtime_error("invalid protection profile");
}

} // namespace

void save_protected_snapshot(const ProtectedMemorySnapshot& s,
                             const std::string& path) {
    if (s.rule_profiles.size() != s.rules.size() || s.rule_hashes.size() != s.rules.size())
        throw std::runtime_error("protected snapshot rule metadata mismatch");

    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("cannot open protected snapshot for writing");

        out.write(kMagic.data(), static_cast<std::streamsize>(kMagic.size()));
        write_pod(out, s.version);
        write_pod(out, static_cast<std::uint64_t>(s.interleave_lanes));
        write_pod(out, static_cast<std::uint64_t>(s.rules.size()));
        write_pod(out, static_cast<std::uint64_t>(s.rule_dual_parity.size()));
        write_pod(out, static_cast<std::uint64_t>(s.trails.size()));

        for (std::size_t i = 0; i < s.rules.size(); ++i) {
            const auto& r = s.rules[i];
            write_pod(out, static_cast<std::uint64_t>(r.id));
            write_pod(out, static_cast<std::uint64_t>(r.left));
            write_pod(out, static_cast<std::uint64_t>(r.right));
            write_pod(out, static_cast<std::uint64_t>(r.frequency));
            write_pod(out, profile_byte(s.rule_profiles[i]));
            write_pod(out, s.rule_hashes[i]);
        }

        for (const auto& p : s.rule_dual_parity) {
            write_pod(out, static_cast<std::uint64_t>(p.begin_index));
            write_pod(out, static_cast<std::uint64_t>(p.count));
            write_bytes(out, p.p_id); write_bytes(out, p.q_id);
            write_bytes(out, p.p_left); write_bytes(out, p.q_left);
            write_bytes(out, p.p_right); write_bytes(out, p.q_right);
            write_bytes(out, p.p_frequency); write_bytes(out, p.q_frequency);
        }

        for (const auto& t : s.trails) {
            write_pod(out, profile_byte(t.profile));
            write_pod(out, static_cast<std::uint64_t>(t.block_size));
            write_pod(out, t.trail_hash);
            write_pod(out, static_cast<std::uint64_t>(t.trail.size()));
            for (auto symbol : t.trail) write_pod(out, static_cast<std::uint64_t>(symbol));
            write_pod(out, static_cast<std::uint64_t>(t.block_hashes.size()));
            for (auto h : t.block_hashes) write_pod(out, h);
            write_pod(out, static_cast<std::uint64_t>(t.dual_parity.size()));
            for (const auto& p : t.dual_parity) {
                write_pod(out, static_cast<std::uint64_t>(p.begin_index));
                write_pod(out, static_cast<std::uint64_t>(p.count));
                write_bytes(out, p.p); write_bytes(out, p.q);
            }
        }
    }

    const auto payload_size = file_size(path);
    const auto checksum = checksum_prefix(path, payload_size);
    std::ofstream out(path, std::ios::binary | std::ios::app);
    if (!out) throw std::runtime_error("cannot append protected snapshot checksum");
    write_pod(out, checksum);
}

ProtectedMemorySnapshot load_protected_snapshot(const std::string& path) {
    verify_file_checksum(path);

    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open protected snapshot for reading");

    std::array<char, 8> magic{};
    in.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (!in || magic != kMagic) throw std::runtime_error("invalid protected snapshot magic");

    ProtectedMemorySnapshot s;
    s.version = read_pod<std::uint32_t>(in);
    if (s.version != 2) throw std::runtime_error("unsupported protected snapshot version");
    s.interleave_lanes = static_cast<std::size_t>(read_pod<std::uint64_t>(in));

    const auto rule_count = read_pod<std::uint64_t>(in);
    const auto rule_parity_count = read_pod<std::uint64_t>(in);
    const auto trail_count = read_pod<std::uint64_t>(in);
    if (rule_count > 100000000ULL || trail_count > 100000000ULL)
        throw std::runtime_error("protected snapshot count exceeds safety limit");

    s.rules.reserve(static_cast<std::size_t>(rule_count));
    s.rule_profiles.reserve(static_cast<std::size_t>(rule_count));
    s.rule_hashes.reserve(static_cast<std::size_t>(rule_count));
    for (std::uint64_t i = 0; i < rule_count; ++i) {
        AdaptiveRule r;
        r.id = read_pod<std::uint64_t>(in);
        r.left = read_pod<std::uint64_t>(in);
        r.right = read_pod<std::uint64_t>(in);
        const auto f = read_pod<std::uint64_t>(in);
        if (f > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
            throw std::runtime_error("frequency overflow");
        r.frequency = static_cast<std::size_t>(f);
        s.rules.push_back(r);
        s.rule_profiles.push_back(read_profile(read_pod<std::uint8_t>(in)));
        s.rule_hashes.push_back(read_pod<std::uint64_t>(in));
    }

    s.rule_dual_parity.reserve(static_cast<std::size_t>(rule_parity_count));
    for (std::uint64_t i = 0; i < rule_parity_count; ++i) {
        RuleDualParityGroup p;
        p.begin_index = static_cast<std::size_t>(read_pod<std::uint64_t>(in));
        p.count = static_cast<std::size_t>(read_pod<std::uint64_t>(in));
        p.p_id = read_bytes(in); p.q_id = read_bytes(in);
        p.p_left = read_bytes(in); p.q_left = read_bytes(in);
        p.p_right = read_bytes(in); p.q_right = read_bytes(in);
        p.p_frequency = read_bytes(in); p.q_frequency = read_bytes(in);
        s.rule_dual_parity.push_back(p);
    }

    s.trails.reserve(static_cast<std::size_t>(trail_count));
    for (std::uint64_t i = 0; i < trail_count; ++i) {
        ProtectedTrailState t;
        t.profile = read_profile(read_pod<std::uint8_t>(in));
        t.block_size = static_cast<std::size_t>(read_pod<std::uint64_t>(in));
        t.trail_hash = read_pod<std::uint64_t>(in);
        const auto n = read_pod<std::uint64_t>(in);
        if (n > 1000000000ULL) throw std::runtime_error("protected trail too large");
        t.trail.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t j = 0; j < n; ++j) t.trail.push_back(read_pod<std::uint64_t>(in));
        const auto hc = read_pod<std::uint64_t>(in);
        t.block_hashes.reserve(static_cast<std::size_t>(hc));
        for (std::uint64_t j = 0; j < hc; ++j) t.block_hashes.push_back(read_pod<std::uint64_t>(in));
        const auto pc = read_pod<std::uint64_t>(in);
        t.dual_parity.reserve(static_cast<std::size_t>(pc));
        for (std::uint64_t j = 0; j < pc; ++j) {
            TrailDualParityBlock p;
            p.begin_index = static_cast<std::size_t>(read_pod<std::uint64_t>(in));
            p.count = static_cast<std::size_t>(read_pod<std::uint64_t>(in));
            p.p = read_bytes(in); p.q = read_bytes(in);
            t.dual_parity.push_back(p);
        }
        s.trails.push_back(std::move(t));
    }

    return s;
}

} // namespace bit_analyze
