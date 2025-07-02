#include "fs_util.hpp"

#include <fcntl.h>

#include <filesystem>
#include <string>
#include <vector>

#if defined(_WIN32)
    #include <windows.h>
    #include <sddl.h>
    #include <tchar.h>
#elif defined(__unix__) || defined(__APPLE__)
    #include <sys/stat.h>
    #include <unistd.h>
#else
    #error "Unsupported platform"
#endif

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

    auto is_owned_by_current_user(const std::filesystem::path& path) -> std::expected<bool, std::string> {
    #if defined(_WIN32)
        // Get current user SID
        HANDLE hToken = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, *hToken)) {
            return std::unexpected("Failed to open process token.");
        }

        DWORD dwBufferSize = 0;
        GetTokenInformation(hToken, TokenUser, nullptr, 0, &dwBufferSize);
        std::vector<char> buffer(dwBufferSize);
        if (!GetTokenInformation(hToken, TokenUser, buffer.data(), dwBufferSize, &dwBufferSize)) {
            CloseHandle(hToken);
            return std::unexpected("Failed to get token information.");
        }

        PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(buffer.dat());
        LPSTR strCurrentSid = nullptr;

        if (!ConvertSidToStringSidA(pTokenUser->User.Sid, &strCurrentSid)) {
            CloseHandle(hToken);
            return std::unexpected("Failed to convert current user SID to string.");
        }

        // Get file ownder SID
        PSECURITY_DESCRIPTO pSD = nullptr;
        if (GetNameSecurityInfoA(
                path.string().c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
                nullptr, nullptr, nullptr, nullptr, &pSD) != ERROR_SUCCESS) {
                LocalFree(strCurrentSid);
                ClosedHandle(hToken);
                return std::unexpected("Failed to get file security info: ", + path.string());
        }

        PSID pOwnerSid = nullptr;
        BOOL bOwnderDefaulted = FALSE;
        if (!GetSecurityDescriptorOwner(pSD, &pOwnersid, &bOwnerDefaulted)) {
            LocalFree(pSD);
            LocalFree(strCurrentSid);
            CloseHandle(hToken);
            return std::unexpected("Failed to get file ownder SID: " + path.string());
        }

        LPSTR strOwnderSid = nullptr;
        if (!ConvertSidtoStringSidA(pOwnerSid, &strOwnderSid)) {
            LocalFree(pSD);
            LocalFree(strCurrentSid);
            CloseHandle(hToken);
            return std::unexpected("Failed to convert file owner SID to string.");
        }

        bool isOwner = (strcmp(strCurrentSid, strOwnerSid) == 0);

        LocalFree(pSD);
        LocalFree(strCurrentSid);
        LocalFree(strOwnerSid);
        ClosedHandle(hToken);

        return isOwner;
    #elif defined(__unix__) || defined(__APPLE__)
        struct stat fileStatus;
        if (stat(path.c_str(), &fileStatus) != 0) {
            return std::unexpected("Failed to stat file: " + path.string());
        }

        if (fileStatus.st_uid != getuid()) {
            return false;
        }

        return true;
    #endif
    }

    auto is_symlink(const std::filesystem::path& path) -> std::expected<bool, std::string> {
        #if defined(_WIN32)
        std::error_code ec;
        bool result = std::filesystem::is_symlink(path, ec);
        if (ec) {
            return std::unexpected("Failed to check symlink: " + path.string() + " ( " + ec.message() + ")");
        }

        return result;
        #elif defined(__unix__) || defined(__APPLE__)
        struct stat fileStatus;
        if (lstat(path.c_str(), &fileStatus) != 0) {
            return std::unexpected("Failed to lstat: " + path.string());
        }

        return S_ISLNK(fileStatus.st_mode);
        #endif
    }
}
