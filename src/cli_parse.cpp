#include "cli_parse.hpp"
#include <set>
#include <sstream>

void print_usage(std::ostream& outputstream) {
    outputstream << "Usage:\n"
       << "  command_parser -h <algo> -t <text> [-o <output>]\n"
       << "  command_parser -h <algo> -f <file> [-o <output>]\n"
       << "  command_parser -v -h <algo> -t <text> -e <expected> [-o <output>]\n"
       << "  command_parser -v -h <algo> -f <file> -e <expected> [-o <output>]\n"
       << "Options:\n"
       << "  -h <algo>      Hash algorithm: md5 | sha1 | sha256\n"
       << "  -t <text>      Text to hash\n"
       << "  -f <file>      File to hash\n"
       << "  -v             Verification mode\n"
       << "  -e <expected>  Expected hash (verify only)\n"
       << "  -o <output>    Output file (optional)\n"
       << "  --help, -H     Show this help\n";
}

//--- Helper functions for clarity ---//
namespace {

struct ParseResult {
    ParsedArgs parsed;
    std::set<std::string_view> seen_flags;
};

auto is_help_flag(std::string_view arg, std::size_t index, const std::vector<std::string_view>& args) -> bool{
    return (arg == "--help" ||
            arg == "-H" ||
            (arg == "-h" && (index + 1 == args.size() || args[index + 1].starts_with('-'))));
}

auto handle_flag(ParseResult& result, const std::vector<std::string_view>& args, std::size_t& index) -> std::expected<void, std::string>{
    auto& parsed = result.parsed;
    auto& seen_flags = result.seen_flags;
    auto arg = args[index];

    // Detect duplicate flag (except -o, last wins)
    if (arg.starts_with('-') && arg.size() == 2) {
        if (arg != "-o" && seen_flags.contains(arg)) {
            std::ostringstream oss;
            oss << "Duplicate flag detected: " << arg << "\n";
            return std::unexpected(oss.str());
        }
        seen_flags.insert(arg);
    }

    if (arg == "-v") {
        parsed.type = CommandType::Verify;
        return {};
    }
    if (arg == "-h" && index + 1 < args.size() && !args[index + 1].starts_with('-')) {
        parsed.algo = parse_algo(args[++index]);
        return {};
    }
    if (arg == "-t" && index + 1 < args.size()) {
        parsed.text = std::string(args[++index]);
        return {};
    }
    if (arg == "-f" && index + 1 < args.size()) {
        parsed.file = std::filesystem::path(args[++index]);
        return {};
    }
    if (arg == "-e" && index + 1 < args.size()) {
        parsed.expected = std::string(args[++index]);
        return {};
    }
    if (arg == "-o" && index + 1 < args.size()) {
        parsed.output = std::filesystem::path(args[++index]);
        return {};
    }
    std::ostringstream oss;
    oss << "Unknown argument: " << arg << "\n";
    return std::unexpected(oss.str());
}

auto parse_flags(const std::vector<std::string_view>& args) -> std::expected<ParsedArgs, std::string>{
    ParseResult result;
    for (std::size_t index = 1; index < args.size(); ++index) {
        if (is_help_flag(args[index], index, args)) {
            return std::unexpected("help");
        }
        auto flag_result = handle_flag(result, args, index);
        if (!flag_result) {
            return std::unexpected(flag_result.error());
        }
    }
    return result.parsed;
}

} // namespace

auto parse_cli(const std::vector<std::string_view>& args)
    -> std::expected<std::variant<HashCmd, VerifyCmd, CommandType>, std::string>
{
    if (args.size() <= 1) {
        std::ostringstream oss;
        oss << "No arguments provided.\n";
        return std::unexpected(oss.str());
    }

    auto flags_result = parse_flags(args);
    if (!flags_result) {
        // Special string "help" means user requested help, not an error
        if (flags_result.error() == "help") {
            return CommandType::Help;
        }
        return std::unexpected(flags_result.error());
    }

    const auto& parsed = flags_result.value();

    // Required argument checks
    if (!parsed.algo || parsed.algo.value() == HashAlgo::Unknown) {
        std::ostringstream oss;
        oss << "Missing or invalid hash algorithm (-h md5|sha1|sha256).\n";
        return std::unexpected(oss.str());
    }
    if (!(parsed.text || parsed.file)) {
        std::ostringstream oss;
        oss << "Specify input with -t <text> or -f <file>.\n";
        return std::unexpected(oss.str());
    }
    if (parsed.text && parsed.file) {
        std::ostringstream oss;
        oss << "Cannot specify both -t and -f.\n";
        return std::unexpected(oss.str());
    }

    // Required flags first: if verify, must have -e
    if (parsed.type == CommandType::Verify) {
        if (!parsed.expected) {
            std::ostringstream oss;
            oss << "Missing expected hash (-e) for verify.\n";
            return std::unexpected(oss.str());
        }
        return VerifyCmd{parsed.algo.value(), parsed.text, parsed.file, parsed.expected.value()};
    }

    return HashCmd{parsed.algo.value(), parsed.text, parsed.file, parsed.output};
}
