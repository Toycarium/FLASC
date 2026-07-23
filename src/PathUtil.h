#pragma once

#include <filesystem>
#include <string>

namespace flasc {

// Converts a UTF-8 encoded std::string (what we treat as FLASC's own internal
// string encoding everywhere, including console I/O once InitConsoleUtf8() has
// been called) into an fs::path. On Windows this goes through an explicit
// UTF-8 -> UTF-16 conversion rather than fs::path's default narrow-string
// constructor, which instead assumes the system ANSI codepage - a mismatch
// that silently breaks any path containing non-ASCII characters (Cyrillic,
// etc), even though it looks correct when echoed back to the same console.
std::filesystem::path ToPath(const std::string& utf8);

// The inverse: turns an fs::path back into a UTF-8 std::string, safe to print
// to a console that's been switched to the UTF-8 codepage.
std::string FromPath(const std::filesystem::path& path);

// On Windows, switches the console's input/output codepage to UTF-8 so
// std::cin/std::cout exchange UTF-8 bytes with the terminal, matching what
// ToPath/FromPath assume. No-op on other platforms (console I/O is UTF-8
// natively there already).
void InitConsoleUtf8();

// Reads one line of console input, returned as UTF-8. On Windows this reads
// via the wide-character console API (std::wcin) rather than std::cin -
// narrow console input negotiates multi-byte encodings (like UTF-8) through
// the console codepage, which has long-standing bugs with pasted (as opposed
// to typed) multi-byte text on several Windows versions; reading UTF-16
// directly sidesteps that class of bug entirely. Returns false at end of
// input, mirroring std::getline's fail-bit behaviour.
bool ReadLineUtf8(std::string& out);

#if defined(_WIN32)
// Converts one wide command-line argument (as delivered by wmain/argv) to
// UTF-8. Command-line arguments arrive via CommandLineToArgvW with full
// Unicode fidelity and no console I/O involved at all, making this a fully
// reliable alternative path for supplying a folder/file containing non-ASCII
// characters - e.g. by dragging a folder onto FLASC.exe - if pasting such a
// path into the interactive prompt ever proves unreliable on a given system.
std::string WideArgToUtf8(const wchar_t* arg);
#endif

} // namespace flasc
