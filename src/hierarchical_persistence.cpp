#include "bit_analyze/hierarchical_persistence.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace bit_analyze {
namespace {

constexpr std::array<char, 8> kMagic{{'B','I','T','H','I','E','R','1'}};
constexpr std::uint32_t kVersion = 1;
constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
constexpr std::uint64_t kMaxRelations = 100000000ULL;

template <typename T>
void write_pod(std::ofstream& out, const T& value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    if (!out) throw std::runtime_error("failed to write hierarchical state");
}

template <typename T>
T read_pod(std::ifstream& in) {
    T value{};
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!in) throw std::runtime_error("truncated hierarchical state");
    return value;
}

std::uint64_t file_size(const std::string& path) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec) throw std::runtime_error("cannot stat hierarchical state");
    return static_cast<std::uint64_t>(size);
}

std::uint64_t checksum_prefix(const std::string& path, std::uint64_t bytes) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open hierarchical state for checksum");
    std::array<char, 8192> buffer{};
    std::uint64_t remaining = bytes;
    std::uint64_t hash = kFnvOffset;
    while (remaining) {
        const auto want = static_cast<std::streamsize>(
            remaining < buffer.size() ? remaining : static_cast<std::uint64_t>(buffer.size()));
        in.read(buffer.data(), want);
        const auto got = in.gcount();
        if (got <= 0) throw std::runtime_error("truncated hierarchical state during checksum");
        for (std::streamsize i = 0; i < got; ++i) {
            hash ^= static_cast<std::uint8_t>(buffer[static_cast<std::size_t>(i)]);
            hash *= kFnvPrime;
        }
        remaining -= static_cast<std::uint64_t>(got);
    }
    return hash;
}

void verify_checksum(const std::string& path) {
    const auto size = file_size(path);
    if (size < sizeof(std::uint64_t))
        throw std::runtime_error("hierarchical state missing checksum");
    const auto payload_size = size - sizeof(std::uint64_t);
    std::ifstream in(path, std::ios::binary);
    in.seekg(static_cast<std::streamoff>(payload_size));
    const auto stored = read_pod<std::uint64_t>(in);
    const auto actual = checksum_prefix(path, payload_size);
    if (stored != actual)
        throw std::runtime_error("hierarchical state checksum mismatch");
}

} // namespace

void save_hierarchical_state(const HierarchicalMemory& memory, const std::string& path) {
    const std::filesystem::path final_path(path);
    if (final_path.has_parent_path())
        std::filesystem::create_directories(final_path.parent_path());
    const auto tmp_path = final_path.string() + ".tmp";

    {
        std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("cannot open hierarchical state for writing");
        out.write(kMagic.data(), static_cast<std::streamsize>(kMagic.size()));
        write_pod(out, kVersion);
        const auto count = static_cast<std::uint64_t>(memory.relations().size());
        write_pod(out, count);
        for (const auto& node : memory.relations()) {
            write_pod(out, static_cast<std::uint64_t>(node.id));
            write_pod(out, static_cast<std::uint64_t>(node.left));
            write_pod(out, static_cast<std::uint64_t>(node.right));
            write_pod(out, node.layer);
            write_pod(out, node.ref_count);
        }
    }

    const auto payload_size = file_size(tmp_path);
    const auto checksum = checksum_prefix(tmp_path, payload_size);
    {
        std::ofstream out(tmp_path, std::ios::binary | std::ios::app);
        if (!out) throw std::runtime_error("cannot append hierarchical state checksum");
        write_pod(out, checksum);
    }

    std::error_code ec;
    std::filesystem::remove(final_path, ec);
    ec.clear();
    std::filesystem::rename(tmp_path, final_path, ec);
    if (ec) {
        std::filesystem::remove(tmp_path);
        throw std::runtime_error("cannot install hierarchical state");
    }
}

void load_hierarchical_state(const std::string& path, HierarchicalMemory& memory) {
    verify_checksum(path);
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open hierarchical state");

    std::array<char, 8> magic{};
    in.read(magic.data(), static_cast<std::streamsize>(magic.size()));
    if (!in || magic != kMagic) throw std::runtime_error("invalid hierarchical state magic");
    const auto version = read_pod<std::uint32_t>(in);
    if (version != kVersion) throw std::runtime_error("unsupported hierarchical state version");
    const auto count = read_pod<std::uint64_t>(in);
    if (count > kMaxRelations) throw std::runtime_error("hierarchical relation count exceeds safety limit");

    std::vector<RelationNode> relations;
    relations.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t i = 0; i < count; ++i) {
        RelationNode node;
        node.id = read_pod<std::uint64_t>(in);
        node.left = read_pod<std::uint64_t>(in);
        node.right = read_pod<std::uint64_t>(in);
        node.layer = read_pod<std::uint32_t>(in);
        node.ref_count = read_pod<std::uint64_t>(in);
        relations.push_back(node);
    }
    memory.restore_relations(relations);
}

} // namespace bit_analyze
