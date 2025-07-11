#pragma once

#include <filesystem>
#include <expected>
#include <string>

namespace fsutil {
    auto is_owned_by_current_user(const std::filesystem::path& path) -> std::expected<bool, std::string>;
    auto is_symlink(const std::filesystem::path& path) -> std::expected<bool, std::string>;
}