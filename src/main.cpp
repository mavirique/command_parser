#include "cli_parse.hpp"
#include "hash_util.hpp"
#include "fs_util.hpp"

#include <iostream>
#include <vector>
#include <string_view>

int main(int argc, char* argv[]) {
    std::vector<std::string_view> args(argv, argv + argc);

    auto res = parse_cli(args);
    if (!res) {
        std::cerr << "Error: " << res.error() << "\n";
        print_usage(std::cerr);
        return 1;
    }

    if (std::holds_alternative<CommandType>(res.value())) {
        print_usage();
        return 0;
    }

    auto dispatch = [&](auto&& cmd) {
        auto outcome = perform_command(cmd);
        if (!outcome) {
            std::cerr << "Error: " << outcome.error() << "\n";
            return 1;
        }
        return 0;
    };

    if (std::holds_alternative<HashCmd>(res.value()))
        return dispatch(std::get<HashCmd>(res.value()));
    else
        return dispatch(std::get<VerifyCmd>(res.value()));
}
