#include "hash_util.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <array>
#include <cstring>
#include <openssl/evp.h>

auto read_file_binary(const std::filesystem::path& file)
    -> std::expected<std::vector<std::byte>, std::string>
{
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs) {
        return std::unexpected("Failed to open file: " + file.string());
    }
    std::vector<char> buf(std::istreambuf_iterator<char>(ifs), {});
    std::vector<std::byte> bytes(buf.size());
    std::memcpy(bytes.data(), buf.data(), buf.size());
    return bytes;
}

auto hash_data(HashAlgo algo, std::span<const std::byte> data)
    -> std::expected<std::string, std::string>
{
    const EVP_MD* md_type = nullptr;
    switch (algo) {
        case HashAlgo::Md5:    md_type = EVP_md5(); break;
        case HashAlgo::Sha1:   md_type = EVP_sha1(); break;
        case HashAlgo::Sha256: md_type = EVP_sha256(); break;
        default:
            return std::unexpected("Unsupported hash algorithm.");
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_len = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        return std::unexpected("EVP_MD_CTX_new failed");
    }

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
    for (std::size_t i = 0; i < digest_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return oss.str();
}

auto perform_command(const HashCmd& cmd) -> std::expected<void, std::string> {
    std::vector<std::byte> data;

    // Input: text or file
    if (cmd.text) {
        const auto* ptr = reinterpret_cast<const std::byte*>(cmd.text->data());
        data.assign(ptr, ptr + cmd.text->size());
    } else if (cmd.file) {
        auto result = read_file_binary(*cmd.file);
        if (!result) {
            return std::unexpected("File error: " + result.error());
        }
        data = std::move(result.value());
    } else {
        return std::unexpected("No input (-t or -f) provided.");
    }

    auto hash = hash_data(cmd.algo, data);
    if (!hash) {
        return std::unexpected("Hash error: " + hash.error());
    }

    if (cmd.output) {
        std::ofstream ofs(*cmd.output, std::ios::out | std::ios::trunc);
        if (!ofs) {
            return std::unexpected("Failed to write output: " + cmd.output->string());
        }
        ofs << hash.value() << '\n';
    } else {
        std::cout << "Hash: " << hash.value() << std::endl;
    }
    return {};
}

auto perform_command(const VerifyCmd& cmd) -> std::expected<void, std::string> {
    std::vector<std::byte> data;

    if (cmd.text) {
        const auto* ptr = reinterpret_cast<const std::byte*>(cmd.text->data());
        data.assign(ptr, ptr + cmd.text->size());
    } else if (cmd.file) {
        auto result = read_file_binary(*cmd.file);
        if (!result) {
            return std::unexpected("File error: " + result.error());
        }
        data = std::move(result.value());
    } else {
        return std::unexpected("No input (-t or -f) provided for verification.");
    }

    auto actual = hash_data(cmd.algo, data);
    if (!actual) {
        return std::unexpected("Hash error: " + actual.error());
    }

    if (actual.value() == cmd.expected) {
        std::cout << "The hash: " << cmd.expected << std::endl;
        std::cout << "Is Verified" << std::endl;
        return {};
    }
    std::cout << "Hash Vefication FAIL" << std::endl;
    std::cout << "Expected: " << cmd.expected << '\n';
    std::cout << "Actual  : " << actual.value() << '\n';
    return std::unexpected("Hash verification failed.");
}
