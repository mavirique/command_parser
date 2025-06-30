#pragma once

#include <filesystem>

namespace fsutil {
    auto open_and_check_owned_by_user(const std::filesystem::path& path) -> int;
}