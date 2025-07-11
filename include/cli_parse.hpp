#pragma once
#include <vector>
#include <string_view>
#include <optional>
#include <filesystem>
#include <expected>
#include <iostream>
#include <variant>
#include "command.hpp"
#include "hash_util.hpp"

// Structs already defined elsewhere in your project:
// enum class HashAlgo { MD5, SHA1, SHA256, Unknown };
// struct HashCmd { HashAlgo algo; std::optional<std::string> text; std::optional<std::filesystem::path> file; std::optional<std::filesystem::path> output; };
// struct VerifyCmd { HashAlgo algo; std::optional<std::string> text; std::optional<std::filesystem::path> file; std::string expected; };

enum class CommandType { Hash, Verify, Help };

struct ParsedArgs {
    CommandType type = CommandType::Hash;
    std::optional<HashAlgo> algo;
    std::optional<std::string> text;
    std::optional<std::filesystem::path> file;
    std::optional<std::string> expected;
    std::optional<std::filesystem::path> output;
};

auto parse_cli(const std::vector<std::string_view>& args) -> std::expected<std::variant<HashCmd, VerifyCmd, CommandType>, std::string>;

void print_usage(std::ostream& outputstream = std::cout);
