#include "PathUtil.h"

#include <iostream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

namespace flasc {

#if defined(_WIN32)

// Not in an anonymous namespace (unlike a typical "private helper" would be)
// simply so ReadLineUtf8() further down in this same file can reuse WideToUtf8
// - neither is declared in PathUtil.h, so both stay internal to this
// translation unit regardless.
std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty())
        return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                                    nullptr, 0);
    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(),
                         size);
    return wide;
}

std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty())
        return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                                    nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(),
                         size, nullptr, nullptr);
    return utf8;
}

std::filesystem::path ToPath(const std::string& utf8) {
    return std::filesystem::path(Utf8ToWide(utf8));
}

std::string FromPath(const std::filesystem::path& path) {
    return WideToUtf8(path.wstring());
}

void InitConsoleUtf8() {
    // Output codepage: narrow std::cout + CP_UTF8 is reliable on modern
    // Windows. Console *input* codepage negotiation for multi-byte encodings
    // has long-standing bugs with pasted (as opposed to typed) text on
    // several Windows versions, so we deliberately don't lean on SetConsoleCP
    // for input at all - ReadLineUtf8() reads via the wide API instead,
    // sidestepping that class of bug entirely rather than working around it.
    SetConsoleOutputCP(CP_UTF8);

    // Required for std::wcin to actually bind to the console correctly and
    // read UTF-16 - without this, wide console input doesn't work at all
    // (fails outright rather than partially), regardless of anything else.
    // Only stdin is switched; stdout stays in its normal mode since we keep
    // using narrow std::cout (paired with the UTF-8 output codepage above).
    _setmode(_fileno(stdin), _O_U16TEXT);
}

bool ReadLineUtf8(std::string& out) {
    std::wstring line;
    if (!std::getline(std::wcin, line)) {
        out.clear();
        return false;
    }
    out = WideToUtf8(line);
    return true;
}

std::string WideArgToUtf8(const wchar_t* arg) {
    return WideToUtf8(std::wstring(arg));
}

#else

std::filesystem::path ToPath(const std::string& utf8) {
    return std::filesystem::path(utf8);
}

std::string FromPath(const std::filesystem::path& path) {
    return path.string();
}

void InitConsoleUtf8() {
    // POSIX terminals are UTF-8 natively - nothing to do.
}

bool ReadLineUtf8(std::string& out) {
    return static_cast<bool>(std::getline(std::cin, out));
}

#endif

} // namespace flasc
