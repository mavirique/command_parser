#pragma once
#include "command.hpp"
#include <string_view>
#include <vector>
#include <expected>

// Parses command-line arguments into a Command
auto parse_cli(const std::vector<std::string_view>& args)
    -> std::expected<Command, std::string>;

// Performs the parsed command (hash, verify)
auto perform_command(const Command& cmd)
    -> std::expected<void, std::string>;
