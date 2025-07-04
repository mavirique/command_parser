#pragma once
#include "hash_algo.hpp"
#include "command.hpp"
#include <filesystem>
#include <expected>
#include <span>
#include <string>
#include <vector>

// Read entire file as binary blob
auto read_file_binary(const std::filesystem::path& file)
    -> std::expected<std::vector<std::byte>, std::string>;

// Hash a data buffer using selected algo (OpenSSL)
auto hash_data(HashAlgo algo, std::span<const std::byte> data)
    -> std::expected<std::string, std::string>;

auto perform_command(const HashCmd& cmd) -> std::expected<void, std::string>;
auto perform_command(const VerifyCmd& cmd) -> std::expected<void, std::string>;
    