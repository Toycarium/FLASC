#include "IvamRunner.h"

#include <algorithm>
#include <filesystem>
#include <vector>

#include "PathUtil.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace flasc {
namespace IvamRunner {

bool ExistsIn(const std::string& folder) {
    std::error_code ec;
    return fs::is_regular_file(ToPath(folder) / "ivam.exe", ec);
}

namespace {

std::string ToLower(std::string s) {
    for (auto& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool EndsWithLower(const std::string& lowerName, const std::string& lowerSuffix) {
    return lowerName.size() >= lowerSuffix.size() &&
           lowerName.compare(lowerName.size() - lowerSuffix.size(), lowerSuffix.size(),
                              lowerSuffix) == 0;
}

// Exact list ivam.exe itself matches against (GlobFiles / ProcessMetadataFiles
// in its main.cpp), case-insensitively.
const std::vector<std::string>& RecognisedSuffixes() {
    static const std::vector<std::string> suffixes = {
        "categories.dat15", "sounds.dat15", "game.dat16",
        "effects.dat11",    "curves.dat12", "speech.dat",
    };
    return suffixes;
}

} // namespace

std::vector<ProcessedFile> CheckProcessed(const std::string& folder, bool genMode) {
    std::vector<ProcessedFile> results;

    for (const auto& name : ListFiles(folder)) {
        std::string lowerName = ToLower(name);
        bool recognised = false;
        for (const auto& suffix : RecognisedSuffixes()) {
            if (EndsWithLower(lowerName, suffix)) {
                recognised = true;
                break;
            }
        }
        if (!recognised)
            continue;

        ProcessedFile pf;
        pf.inputName = name;
        pf.expectedOutputName = name + (genMode ? ".GEN" : ".json");

        std::error_code ec;
        pf.outputExists = fs::is_regular_file(ToPath(folder) / pf.expectedOutputName, ec);

        results.push_back(std::move(pf));
    }

    return results;
}

#if defined(_WIN32)

RunResult Run(const std::string& ivamFolder, bool genMode) {
    RunResult result;

    // Resolve to absolute first: everything below is built from this, so it
    // can't end up inconsistent between "where the exe is" and "what cwd the
    // child starts in" regardless of whether the caller passed a relative or
    // absolute folder.
    fs::path absFolder = fs::absolute(ToPath(ivamFolder));

    // CreateProcessW throughout - CreateProcessA would convert its narrow
    // arguments via the system ANSI codepage, which silently mangles any
    // non-ASCII (Cyrillic, etc) path, same issue PathUtil exists to avoid
    // everywhere else.
    std::wstring exeW = (absFolder / L"ivam.exe").wstring();
    std::wstring cmdLine = L"\"" + exeW + L"\"";
    if (genMode)
        cmdLine += L" gen";
    std::wstring workingDirW = absFolder.wstring();

    // CreateProcessW may write into this buffer, so it must be mutable.
    std::vector<wchar_t> cmdLineBuf(cmdLine.begin(), cmdLine.end());
    cmdLineBuf.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessW(
        nullptr,               // use the command line to find the executable
        cmdLineBuf.data(),
        nullptr, nullptr,      // default process/thread security attributes
        FALSE,                 // don't inherit handles
        0,                     // no special creation flags
        nullptr,               // inherit the current environment
        workingDirW.c_str(),   // <- this is what points ivam at the right folder
        &si, &pi);

    if (!ok) {
        result.started = false;
        result.error = "CreateProcess failed (error code " + std::to_string(GetLastError()) + ")";
        return result;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    result.started = true;
    result.exitCode = static_cast<int>(exitCode);
    return result;
}

#else

// POSIX fallback, used only so this logic can be exercised outside Windows
// while developing/testing - FLASC only ever ships for Windows (ivam.exe is
// a Windows binary), so the CreateProcessW path above is the real one.
RunResult Run(const std::string& ivamFolder, bool genMode) {
    RunResult result;

    fs::path absFolder = fs::absolute(ToPath(ivamFolder));
    std::string absFolderStr = absFolder.string();
    std::string exePath = (absFolder / "ivam.exe").string();

    pid_t pid = fork();
    if (pid < 0) {
        result.started = false;
        result.error = "fork() failed";
        return result;
    }

    if (pid == 0) {
        if (chdir(absFolderStr.c_str()) != 0)
            _exit(127);

        if (genMode)
            execl(exePath.c_str(), exePath.c_str(), "gen", static_cast<char*>(nullptr));
        else
            execl(exePath.c_str(), exePath.c_str(), static_cast<char*>(nullptr));

        _exit(127); // only reached if exec failed
    }

    int status = 0;
    waitpid(pid, &status, 0);
    result.started = true;
    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return result;
}

#endif

std::vector<std::string> ListFiles(const std::string& dir) {
    std::vector<std::string> files;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(ToPath(dir), ec)) {
        if (entry.is_regular_file())
            files.push_back(FromPath(entry.path().filename()));
    }
    std::sort(files.begin(), files.end());
    return files;
}

} // namespace IvamRunner
} // namespace flasc
