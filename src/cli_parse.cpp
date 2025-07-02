#include "cli_parse.hpp"
#include "hash_util.hpp"
#include "fs_util.hpp"

#include <iostream>
#include <fstream>
#include <span>
#include <filesystem>

namespace {
    constexpr std::size_t kVerifyMinArgs = 5;

    auto parse_verify_args(const std::vector<std::string_view>& args)
        -> std::expected<VerifyCmd, std::string>
    {
        constexpr std::size_t kAlgoIdx = 1;
        constexpr std::size_t kFirstOptIdx = 2;
        if (args.size() < kVerifyMinArgs) {
            return std::unexpected("Usage: verify md5|sha1|sha256 --text|--file ... --expected...");
        }
        HashAlgo algo = parse_algo(args[kAlgoIdx]);
        if (algo == HashAlgo::Unknown) {
            return std::unexpected("Unknown hash algorithm for verify.");
        }
        std::optional<std::string> text;
        std::optional<std::filesystem::path> file;
        std::string expected;
        for (std::size_t i = kFirstOptIdx; i < args.size(); ++i) {
            if (args[i] == "--text" && i + 1 < args.size()) {
                text = std::string(args[++i]);
            } else if (args[i] == "--file" && i + 1 < args.size()) {
                auto path = std::filesystem::path(args[++i]);
                auto owner_result = fsutil::is_owned_by_current_user(path);
                if (!owner_result.has_value()) {
                    std::cerr << "Owner check error: " << owner_result.error() << std::endl;
                } else if (!owner_result.value()) {
                    std::cerr << "File is not owned by the current user." << std::endl;
                }
                
                auto symlink_result = fsutil::is_symlink(path);
                if (!symlink_result.has_value()) {
                    std::cerr << "Symlink check error: " << symlink_result.error() << std::endl;
                } else if (symlink_result.value()) {
                    std::cerr << "File is a symlink (refused for security)." << std::endl;
                }               

                file = path; // only after the check!
            } else if (args[i] == "--expected" && i + 1 < args.size()) {
                expected = std::string(args[++i]);
            }
        }    
        if ((!text && !file) || expected.empty()) {
            return std::unexpected("Missing input or --expected hash.");
        }
        return VerifyCmd{algo, text, file, expected};
    }

    auto parse_hash_args(std::string_view main_cmd, const std::vector<std::string_view>& args)
        -> std::expected<HashCmd, std::string>
    {
        HashAlgo algo = parse_algo(main_cmd);
        if (algo == HashAlgo::Unknown) {
            return std::unexpected("Unknown hash algorithm.");
        }
        std::optional<std::string> text;
        std::optional<std::filesystem::path> file;
        std::optional<std::filesystem::path> output;
        for (std::size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "--text" && i + 1 < args.size()) {
                text = std::string(args[++i]);
            } else if (args[i] == "--file" && i + 1 < args.size()) {           
                auto path = std::filesystem::path(args[++i]);
                auto owner_result = fsutil::is_owned_by_current_user(path);
                if (!owner_result.has_value()) {
                    std::cerr << "Owner check error: " << owner_result.error() << std::endl;
                } else if (!owner_result.value()) {
                    std::cerr << "File is not owned by the current user." << std::endl;
                }
                
                auto symlink_result = fsutil::is_symlink(path);
                if (!symlink_result.has_value()) {
                    std::cerr << "Symlink check error: " << symlink_result.error() << std::endl;
                } else if (symlink_result.value()) {
                    std::cerr << "File is a symlink (refused for security)." << std::endl;
                }    

                file = path; // only after the check!        
            } else if (args[i] == "--output" && i + 1 < args.size()) {
                output = std::filesystem::path(args[++i]);
            }
        }
        if (!text && !file) {
            return std::unexpected("Specify --text or --file.");
        }
        return HashCmd{algo, text, file, output};
    }
} // anonymous namespace

auto parse_cli(const std::vector<std::string_view>& args)
    -> std::expected<Command, std::string>
{
    if (args.empty()) {
        return std::unexpected("Missing command.");
    }

    std::string_view main_cmd = args[0];
    if (main_cmd == "verify") {
        auto res = parse_verify_args(args);
        if (!res) {
            return std::unexpected(res.error());
        }
        return res.value();
    }

    auto res = parse_hash_args(main_cmd, args);
    if (!res) {
        return std::unexpected(res.error());
    }

    return res.value();
}

auto perform_command(const Command& cmd)
    -> std::expected<void, std::string>
{
    if (std::holds_alternative<HashCmd>(cmd)) {
        const auto& hash_cmd = std::get<HashCmd>(cmd);

        std::vector<std::byte> data;
        if (hash_cmd.text) {
            const auto* byte_ptr = reinterpret_cast<const std::byte*>(hash_cmd.text->data());
            data.assign(byte_ptr, byte_ptr + hash_cmd.text->size());
        } else if (hash_cmd.file) {
            auto file_result = read_file_binary(*hash_cmd.file);
            if (!file_result) {
                return std::unexpected(file_result.error());
            }
            data = std::move(file_result.value());
        } else {
            return std::unexpected("No input provided.");
        }

        auto hash_result = hash_data(hash_cmd.algo, data);
        if (!hash_result) {
            return std::unexpected("Hash error: " + hash_result.error());
        }

        if (hash_cmd.output) {
            std::ofstream ofs(*hash_cmd.output, std::ios::out | std::ios::trunc);
            if (!ofs) {
                return std::unexpected("Failed to write output: " + hash_cmd.output->string());
            }
            ofs << hash_result.value() << '\n';
        } else {
            std::cout << hash_result.value() << '\n';
        }
        return {};
    }

    const auto& verify_cmd = std::get<VerifyCmd>(cmd);

    std::vector<std::byte> data;
    if (verify_cmd.text) {
        const auto* byte_ptr = reinterpret_cast<const std::byte*>(verify_cmd.text->data());
        data.assign(byte_ptr, byte_ptr + verify_cmd.text->size());
    } else if (verify_cmd.file) {
        auto file_result = read_file_binary(*verify_cmd.file);
        if (!file_result) {
            return std::unexpected(file_result.error());
        }
        data = std::move(file_result.value());
    } else {
        return std::unexpected("No input provided for verification.");
    }

    auto actual_result = hash_data(verify_cmd.algo, data);
    if (!actual_result) {
        return std::unexpected("Hash error: " + actual_result.error());
    }

    if (actual_result.value() == verify_cmd.expected) {
        std::cout << "OK\n";
        return {};
    }
    std::cout << "FAIL\n";
    std::cout << "Expected: " << verify_cmd.expected << '\n';
    std::cout << "Actual  : " << actual_result.value() << '\n';
    return std::unexpected("Hash verification failed.");
}
