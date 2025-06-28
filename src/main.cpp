#include "hash_util.hpp"
#include "cli_parse.hpp"
#include "command.hpp"
#include "hash_algo.hpp"

#include <iostream>
#include <vector>
#include <string_view>

auto main(int argc, char** argv) -> int {
    try {
        std::vector<std::string_view> args(argv + 1, argv + argc);
        auto cmd = parse_cli(args);
        if (!cmd) {
            std::cerr << "Error: " << cmd.error() << "\n";
            // Print usage, if desired
            return 2;
        }
        auto res = perform_command(*cmd);
        if (!res) {
            std::cerr << "Error: " << res.error() << "\n";
            return 2;
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return 2;
    }
}
