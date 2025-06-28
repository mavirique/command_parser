#pragma once
#include "hash_algo.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <variant>

namespace fs = std::filesystem;

struct HashCmd {
    HashAlgo algo;
    std::optional<std::string> text;
    std::optional<fs::path> file;
    std::optional<fs::path> output;
};

struct VerifyCmd {
    HashAlgo algo;
    std::optional<std::string> text;
    std::optional<fs::path> file;
    std::string expected;
};

using Command = std::variant<HashCmd, VerifyCmd>;
