#include "bit_analyze/structural_stream.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace bit_analyze {
namespace {
std::string signature_for(const std::vector<SymbolId>& trail) {
    std::uint64_t h = 1469598103934665603ULL;
    for (const auto id : trail) {
        for (unsigned shift = 0; shift < 64; shift += 8) {
            h ^= static_cast<std::uint8_t>((id >> shift) & 0xffU);
            h *= 1099511628211ULL;
        }
    }
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << h;
    return out.str();
}
std::string escape_json(const std::string& value) {
    std::ostringstream out;
    for (const char c : value) {
        if (c == '"' || c == '\\') out << '\\' << c;
        else if (c == '\n') out << "\\n";
        else out << c;
    }
    return out.str();
}
template <typename T> void json_array(std::ostringstream& out, const std::vector<T>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) { if (i) out << ','; out << values[i]; }
    out << ']';
}
} // namespace

std::string StructuralEvent::to_json() const {
    std::ostringstream out;
    out << "{\"version\":" << version << ",\"source_id\":\"" << escape_json(source_id) << "\""
        << ",\"sequence\":" << sequence << ",\"byte_offset\":" << byte_offset
        << ",\"byte_length\":" << byte_length << ",\"trail\":";
    json_array(out, trail); out << ",\"relation_ids\":"; json_array(out, relation_ids);
    out << ",\"signature\":\"" << signature << "\",\"resolution\":" << resolution << '}';
    return out.str();
}

StructuralExtractor::StructuralExtractor(HierarchicalMemory& memory, std::uint32_t max_layers)
    : memory_(memory), max_layers_(max_layers) {
    if (max_layers_ == 0) throw std::invalid_argument("max_layers must be > 0");
}
StructuralEvent StructuralExtractor::extract(const std::vector<std::uint8_t>& bytes,
                                             const std::string& source_id,
                                             std::uint64_t sequence,
                                             std::uint64_t byte_offset) {
    const auto encoded = memory_.encode(bytes, max_layers_);
    StructuralEvent event;
    event.source_id = source_id; event.sequence = sequence; event.byte_offset = byte_offset;
    event.byte_length = bytes.size(); event.trail = encoded.trail; event.resolution = max_layers_;
    for (const auto id : event.trail) if (id >= 256) event.relation_ids.push_back(id);
    event.signature = signature_for(event.trail);
    return event;
}

StructuralStream::StructuralStream(StructuralExtractor& extractor, std::size_t window_size,
                                   std::size_t hop_size)
    : extractor_(extractor), window_size_(window_size), hop_size_(hop_size) {
    if (window_size_ == 0 || hop_size_ == 0 || hop_size_ > window_size_)
        throw std::invalid_argument("require 0 < hop_size <= window_size");
    buffer_.reserve(window_size_);
}
std::vector<StructuralEvent> StructuralStream::push(const std::uint8_t* data, std::size_t size,
                                                    const std::string& source_id) {
    if (size && data == nullptr) throw std::invalid_argument("null data");
    std::vector<StructuralEvent> events;
    std::size_t pos = 0;
    while (pos < size) {
        const auto take = std::min(window_size_ - buffer_.size(), size - pos);
        buffer_.insert(buffer_.end(), data + pos, data + pos + take);
        pos += take; consumed_bytes_ += take;
        if (buffer_.size() == window_size_) {
            events.push_back(extractor_.extract(buffer_, source_id, sequence_++, buffer_offset_));
            buffer_.erase(buffer_.begin(), buffer_.begin() + hop_size_);
            buffer_offset_ += hop_size_;
        }
    }
    return events;
}
std::vector<StructuralEvent> StructuralStream::push(const std::vector<std::uint8_t>& data,
                                                    const std::string& source_id) {
    if (data.empty()) return {};
    return push(data.data(), data.size(), source_id);
}
std::vector<StructuralEvent> StructuralStream::emit_ready(const std::string&) { return {}; }
std::vector<StructuralEvent> StructuralStream::flush(const std::string& source_id) {
    std::vector<StructuralEvent> events;
    if (!buffer_.empty()) {
        const auto tail_size = buffer_.size();
        events.push_back(extractor_.extract(buffer_, source_id, sequence_++, buffer_offset_));
        buffer_offset_ += tail_size; buffer_.clear();
    }
    return events;
}
std::size_t StructuralStream::buffered_bytes() const noexcept { return buffer_.size(); }
std::uint64_t StructuralStream::consumed_bytes() const noexcept { return consumed_bytes_; }

} // namespace bit_analyze
