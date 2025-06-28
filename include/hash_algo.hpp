#pragma once
#include <string_view>

enum class HashAlgo { Md5, Sha1, Sha256, Unknown };

// Parse a string ("md5", ...) to HashAlgo
inline auto parse_algo(std::string_view str) -> HashAlgo {
    if (str == "md5")    return HashAlgo::Md5;
    if (str == "sha1")   return HashAlgo::Sha1;
    if (str == "sha256") return HashAlgo::Sha256;
    return HashAlgo::Unknown;
}
