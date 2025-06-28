#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <expected>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <array>
#include <span>
#include <format>
#include <stdexcept>
#include <cstring>
#include <variant>

#include <openssl/evp.h>

namespace fs = std::filesystem;

// --- Hash Algorithm enum ---
enum class HashAlgo { Md5, Sha1, Sha256, Unknown };

// --- Command Structures ---
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

// --- Helper Functions ---
inline auto parse_algo(std::string_view str) -> HashAlgo {
    if (str == "md5")    { return HashAlgo::Md5; }
    if (str == "sha1")   { return HashAlgo::Sha1; }
    if (str == "sha256") { return HashAlgo::Sha256; }
    return HashAlgo::Unknown;
}

inline auto read_file_binary(const fs::path& file) -> std::expected<std::vector<std::byte>, std::string> {
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs) {
        return std::unexpected(std::format("Failed to open file: {}", file.string()));
    }

    std::vector<char> buf(std::istreambuf_iterator<char>(ifs), {});
    std::vector<std::byte> bytes(buf.size());
    std::memcpy(bytes.data(), buf.data(), buf.size());
    return bytes;
}

inline auto hash_data(HashAlgo algo, std::span<const std::byte> data) -> std::expected<std::string, std::string> {
    const EVP_MD* md_type = nullptr;
    switch (algo) {
        case HashAlgo::Md5:    md_type = EVP_md5();    break;
        case HashAlgo::Sha1:   md_type = EVP_sha1();   break;
        case HashAlgo::Sha256: md_type = EVP_sha256(); break;
        default:
            return std::unexpected("Unsupported hash algorithm.");
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
        return std::unexpected("EVP_MD_CTX_new failed");

    if (EVP_DigestInit_ex(ctx, md_type, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return std::unexpected("EVP_DigestInit_ex failed");
    }

    if (EVP_DigestUpdate(ctx, data.data(), data.size_bytes()) != 1) {
        EVP_MD_CTX_free(ctx);
        return std::unexpected("EVP_DigestUpdate failed");
    }

    if (EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return std::unexpected("EVP_DigestFinal_ex failed");
    }

    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    for (size_t i = 0; i < digest_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << int(digest[i]);
    }

    return oss.str();
}


// --- Command Line Parsing ---
inline auto parse_cli(const std::vector<std::string_view>& args) -> std::expected<Command, std::string> {
    if (args.empty()) {
        return std::unexpected("Missing command.");
    }

    std::string_view main_cmd = args[0];
    if (main_cmd == "verify") {
        if (args.size() < 5) {
            return std::unexpected("Usage: verify md5|sha1|sha256 --text|--file ... --expected...");
        }

        HashAlgo algo = parse_algo(args[1]);
        if (algo == HashAlgo::Unknown) {
            return std::unexpected("Unknown hash algorithm for verify.");
        }

        std::optional<std::string> text;
        std::optional<fs::path> file;
        std::string expected;
        for (size_t i = 2; i < args.size(); ++i) {
            if (args[i] == "--text" && i + 1 < args.size()) {
                text = std::string(args[++i]);
            } else if (args[i] == "--file" && i + 1 < args.size()) {
                file = fs::path(args[++i]);
            } else if (args[i] == "--expected" && i + 1 < args.size()) {
                expected = std::string(args[++i]);
            }
        }
        
        if ((!text && !file) || expected.empty()) {
            return std::unexpected("Missing input or --expected hash.");
        }

        return VerifyCmd{algo, text, file, expected};
    } else {
        HashAlgo algo = parse_algo(main_cmd);
        if (algo == HashAlgo::Unknown) {
            return std::unexpected("Unknown hash algorithm.");
        }

        std::optional<std::string> text;
        std::optional<fs::path> file;
        std::optional<fs::path> output;

        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "--text" && i + 1 < args.size()) {
                text = std::string(args[++i]);
            } else if (args[i] == "--file" && i + 1 < args.size()) {
                file = fs::path(args[++i]);
            } else if (args[i] == "--output" && i + 1 < args.size()) {
                output = fs::path(args[++i]);
            }
        }

        if (!text && !file) {
            return std::unexpected("Specify --text or --file. ");
        }

        return HashCmd{algo, text, file, output};
    }
}

inline auto perform_command(const Command& cmd) -> std::expected<void, std::string> {
    if (std::holds_alternative<HashCmd>(cmd)) {
        const auto& c = std::get<HashCmd>(cmd);

        std::vector<std::byte> data;
        if (c.text) {
            const auto* p = reinterpret_cast<const std::byte*>(c.text->data());
            data.assign(p, p + c.text->size());
        } else if (c.file) {
            auto res = read_file_binary(*c.file);
            if (!res)
                return std::unexpected(res.error());
            data = std::move(res.value());
        } else {
            return std::unexpected("No input provided.");
        }

        auto hash = hash_data(c.algo, data);
        if (!hash) {
            return std::unexpected(std::format("Hash error: {}", hash.error()));
        }

        if (c.output) {
            std::ofstream ofs(*c.output, std::ios::out | std::ios::trunc);
            if (!ofs)
                return std::unexpected(std::format("Failed to write output: {}", c.output->string()));
            ofs << hash.value() << '\n';
        } else {
            std::cout << hash.value() << '\n';
        }
        return {};
    } else {
        const auto& v = std::get<VerifyCmd>(cmd);

        std::vector<std::byte> data;
        if (v.text) {
            const auto* p = reinterpret_cast<const std::byte*>(v.text->data());
            data.assign(p, p + v.text->size());
        } else if (v.file) {
            auto res = read_file_binary(*v.file);
            if (!res)
                return std::unexpected(res.error());
            data = std::move(res.value());
        } else {
            return std::unexpected("No input provided for verification.");
        }

        auto actual = hash_data(v.algo, data);
        if (!actual) {
            return std::unexpected(std::format("Hash error: {}", actual.error()));
        }

        if (actual.value() == v.expected) {
            std::cout << "OK\n";
            return {};
        } else {
            std::cout << "FAIL\n";
            std::cout << "Expected: " << v.expected << '\n';
            std::cout << "Actual  : " << actual.value() << '\n';
            return std::unexpected("Hash verification failed.");
        }
    }
}

int main(int argc, char** argv) {
    std::vector<std::string_view> args(argv + 1, argv + argc);
    auto cmd = parse_cli(args);
    if (!cmd) {
        std::cerr << "Error: " << cmd.error() << "\n";
        // print usage...
        return 2;
    }
    auto res = perform_command(*cmd);
    if (!res) {
        std::cerr << "Error: " << res.error() << "\n";
        return 2;
    }
    return 0;
}
