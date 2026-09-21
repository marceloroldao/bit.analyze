#include "bit_analyze/hierarchical_memory.hpp"
#include "bit_analyze/hierarchical_persistence.hpp"
#include "bit_analyze/structural_stream.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Options {
    std::string input;
    std::string source_id;
    std::string batch_list;
    std::string state_in;
    std::string state_out;
    std::size_t window_size{4096};
    std::size_t hop_size{4096};
    std::size_t chunk_size{65536};
    std::uint32_t layers{2};
};

std::size_t parse_size(const std::string& value, const char* name) {
    std::size_t pos = 0;
    const auto parsed = std::stoull(value, &pos);
    if (pos != value.size() || parsed == 0)
        throw std::invalid_argument(std::string(name) + " must be a positive integer");
    return static_cast<std::size_t>(parsed);
}

std::uint32_t parse_layers(const std::string& value) {
    const auto parsed = parse_size(value, "--layers");
    if (parsed > 32) throw std::invalid_argument("--layers must be <= 32");
    return static_cast<std::uint32_t>(parsed);
}

Options parse_args(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&](const char* name) -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(std::string("missing value for ") + name);
            return argv[++i];
        };
        if (arg == "--input") options.input = value("--input");
        else if (arg == "--source-id") options.source_id = value("--source-id");
        else if (arg == "--batch-list") options.batch_list = value("--batch-list");
        else if (arg == "--state-in") options.state_in = value("--state-in");
        else if (arg == "--state-out") options.state_out = value("--state-out");
        else if (arg == "--state") {
            const auto state = value("--state");
            options.state_in = state;
            options.state_out = state;
        }
        else if (arg == "--window") options.window_size = parse_size(value("--window"), "--window");
        else if (arg == "--hop") options.hop_size = parse_size(value("--hop"), "--hop");
        else if (arg == "--chunk-size") options.chunk_size = parse_size(value("--chunk-size"), "--chunk-size");
        else if (arg == "--layers") options.layers = parse_layers(value("--layers"));
        else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage:\n"
                << "  bit_analyze_structural_ingest --input FILE --source-id ID [options]\n"
                << "  bit_analyze_structural_ingest --batch-list TSV [options]\n\n"
                << "Options:\n"
                << "  --state-in FILE    load persistent hierarchical relation state if present\n"
                << "  --state-out FILE   save resulting relation state after full success\n"
                << "  --state FILE       shorthand for the same input/output state path\n"
                << "  --window N         structural window bytes (default 4096)\n"
                << "  --hop N            structural hop bytes (default 4096)\n"
                << "  --chunk-size N     file read chunk bytes (default 65536)\n"
                << "  --layers N         hierarchy layers, 1..32 (default 2)\n"
                << "Batch TSV format: source_id<TAB>absolute_or_relative_path\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown argument: " + arg);
        }
    }

    const bool single = !options.input.empty() || !options.source_id.empty();
    const bool batch = !options.batch_list.empty();
    if (single == batch)
        throw std::invalid_argument("choose exactly one of --input/--source-id or --batch-list");
    if (single && (options.input.empty() || options.source_id.empty()))
        throw std::invalid_argument("--input and --source-id are required together");
    if (options.hop_size > options.window_size)
        throw std::invalid_argument("--hop must be <= --window");
    return options;
}

std::vector<std::pair<std::string, std::string>> read_batch(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open batch list: " + path);
    std::vector<std::pair<std::string, std::string>> items;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(in, line)) {
        ++line_number;
        if (line.empty()) continue;
        const auto tab = line.find('\t');
        if (tab == std::string::npos || tab == 0 || tab + 1 >= line.size())
            throw std::runtime_error("invalid batch TSV at line " + std::to_string(line_number));
        const auto source_id = line.substr(0, tab);
        const auto object_path = line.substr(tab + 1);
        if (source_id.find('\n') != std::string::npos || source_id.find('\r') != std::string::npos ||
            source_id.find('\t') != std::string::npos)
            throw std::runtime_error("invalid source_id in batch TSV");
        items.emplace_back(source_id, object_path);
    }
    if (items.empty()) throw std::runtime_error("batch list is empty");
    return items;
}

std::uint64_t ingest_file(
    bit_analyze::StructuralExtractor& extractor,
    const std::string& source_id,
    const std::string& path,
    const Options& options
) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open input: " + path);

    bit_analyze::StructuralStream stream(extractor, options.window_size, options.hop_size);
    std::vector<std::uint8_t> buffer(options.chunk_size);
    std::uint64_t events = 0;

    while (in) {
        in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const auto got = in.gcount();
        if (got < 0) throw std::runtime_error("negative read count");
        if (got == 0) break;
        const auto batch = stream.push(buffer.data(), static_cast<std::size_t>(got), source_id);
        for (const auto& event : batch) {
            std::cout << event.to_json() << '\n';
            ++events;
        }
    }
    if (!in.eof() && in.fail())
        throw std::runtime_error("failed while reading input: " + path);

    const auto tail = stream.flush(source_id);
    for (const auto& event : tail) {
        std::cout << event.to_json() << '\n';
        ++events;
    }
    return events;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse_args(argc, argv);
        bit_analyze::HierarchicalMemory memory;
        if (!options.state_in.empty() && std::filesystem::exists(options.state_in))
            bit_analyze::load_hierarchical_state(options.state_in, memory);

        bit_analyze::StructuralExtractor extractor(memory, options.layers);
        std::vector<std::pair<std::string, std::string>> inputs;
        if (!options.batch_list.empty())
            inputs = read_batch(options.batch_list);
        else
            inputs.emplace_back(options.source_id, options.input);

        std::uint64_t total_events = 0;
        for (const auto& [source_id, path] : inputs)
            total_events += ingest_file(extractor, source_id, path, options);

        std::cout.flush();
        if (!std::cout) throw std::runtime_error("failed to write StructuralEvent JSONL");

        if (!options.state_out.empty())
            bit_analyze::save_hierarchical_state(memory, options.state_out);

        std::cerr
            << "bit.analyze ingest complete: sources=" << inputs.size()
            << " events=" << total_events
            << " relations=" << memory.relation_count()
            << "\n";
        return 0;
    } catch (const std::exception& exc) {
        std::cerr << "bit.analyze ingest error: " << exc.what() << "\n";
        return 2;
    }
}
