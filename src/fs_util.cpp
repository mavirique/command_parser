#include "fs_util.hpp"

#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fsutil {

    auto open_and_check_owned_by_user(const std::filesystem::path& path) -> int {
        int fileDescriptor = ::open(path.c_str(), O_RDONLY | O_NOFOLLOW);
        if (fileDescriptor < 0) {
            return -1;
        }

        struct stat fileStatus;
        if (::fstat(fileDescriptor, &fileStatus) != 0) {
            ::close(fileDescriptor);
            return -1;
        }

        if (fileStatus.st_uid != ::getuid()) {
            ::close(fileDescriptor);
            return -1;
        }

        return fileDescriptor;
    }
}
