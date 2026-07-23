#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "IniLoader.h"
#include "IvamRunner.h"
#include "Json.h"
#include "JsonMerge.h"
#include "PathUtil.h"
#include "VehicleJsonBuilder.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

std::string GetExeDir() {
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    fs::path exePath(std::wstring(buffer, len));
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    fs::path exePath(len == -1 ? "." : std::string(buffer, static_cast<size_t>(len)));
#endif
    return flasc::FromPath(exePath.parent_path());
}

std::string TrimWhitespace(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Strips one matching pair of leading/trailing double quotes, e.g. from
// Windows Explorer's "Copy as path". Deliberately staged separately from
// whitespace trimming (rather than folding both into one find_first_not_of
// character class): a path ending in a backslash right before the closing
// quote - "C:\Tools\" - is extremely common for folder paths, and a combined
// trim stops at that backslash (it isn't whitespace or a quote) before ever
// reaching the quote, leaving it stuck on. Handling quotes as their own
// explicit step avoids that.
std::string StripQuotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

std::string Trim(const std::string& s) {
    std::string result = TrimWhitespace(s);
    result = StripQuotes(result);
    result = TrimWhitespace(result); // in case there was whitespace just inside the quotes
    return result;
}

bool EndsWithExt(const std::string& s, const std::string& ext) {
    if (s.size() < ext.size())
        return false;
    std::string tail = s.substr(s.size() - ext.size());
    for (auto& c : tail)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    std::string lowerExt = ext;
    for (auto& c : lowerExt)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return tail == lowerExt;
}

// Resolves what the user typed into an actual file path. Tries, in order:
//  1) exactly what they typed (relative to the current working directory, or absolute)
//  2) the same, with the given extension appended, if they left it off
//  3) the same two attempts again, but next to the .exe itself, so a user who just
//     types a bare name finds the file sitting alongside the program without having
//     to type a full path.
std::string ResolvePath(const std::string& rawInput, const std::string& exeDir,
                        const std::string& ext) {
    std::string input = Trim(rawInput);
    if (input.empty())
        return "";

    std::vector<std::string> candidates = {input};
    if (!EndsWithExt(input, ext))
        candidates.push_back(input + ext);

    size_t count = candidates.size();
    for (size_t i = 0; i < count; ++i)
        candidates.push_back(flasc::FromPath(flasc::ToPath(exeDir) / candidates[i]));

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (fs::is_regular_file(flasc::ToPath(candidate), ec))
            return candidate;
    }
    return "";
}

std::string ReadWholeFile(const std::string& path) {
    std::ifstream in(flasc::ToPath(path), std::ios::binary);
    if (!in.is_open())
        throw std::runtime_error("Could not open file: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void WriteOutputFile(const std::string& exeDir, const std::string& fileName,
                     const std::string& contents) {
    fs::path outputDir = flasc::ToPath(exeDir) / "output";
    fs::create_directories(outputDir);
    fs::path outputPath = outputDir / flasc::ToPath(fileName);

    std::ofstream out(outputPath, std::ios::binary);
    out << contents;
    out.close();

    std::cout << "Written to: " << flasc::FromPath(outputPath) << "\n";
}

void ConvertIniToJson(const std::string& exeDir) {
    std::cout << "Path to .ini file: ";
    std::string input;
    flasc::ReadLineUtf8(input);

    std::string path = ResolvePath(input, exeDir, ".ini");
    if (path.empty()) {
        std::cout << "File not found.\n";
        return;
    }

    std::vector<flasc::IniSection> sections;
    try {
        sections = flasc::IniLoader::Load(path);
    } catch (const std::exception& ex) {
        std::cout << "Could not read file: " << ex.what() << "\n";
        return;
    }

    auto root = std::make_unique<flasc::json::JObject>();

    int ok = 0, failed = 0;
    for (const auto& section : sections) {
        try {
            std::string upperName = section.name;
            for (auto& c : upperName)
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

            root->Set(upperName, flasc::VehicleJsonBuilder::BuildVehicleObject(section));
            ++ok;
        } catch (const std::exception& ex) {
            std::cout << "  [" << section.name << "] FAILED: " << ex.what() << "\n";
            ++failed;
        }
    }

    std::cout << "Converted " << ok << " section(s), " << failed << " failed.\n";
    WriteOutputFile(exeDir, "converted.json", root->ToJsonString());
}

void MergeJsonFiles(const std::string& exeDir) {
    std::cout << "Path to .json file to insert: ";
    std::string insertInput;
    flasc::ReadLineUtf8(insertInput);
    std::string insertPath = ResolvePath(insertInput, exeDir, ".json");
    if (insertPath.empty()) {
        std::cout << "File not found.\n";
        return;
    }

    std::cout << "Path to .json file to merge into: ";
    std::string baseInput;
    flasc::ReadLineUtf8(baseInput);
    std::string basePath = ResolvePath(baseInput, exeDir, ".json");
    if (basePath.empty()) {
        std::cout << "File not found.\n";
        return;
    }

    try {
        std::string insertText = ReadWholeFile(insertPath);
        std::string baseText = ReadWholeFile(basePath);

        auto insertEntries = flasc::JsonMerge::SplitTopLevelObject(insertText);
        auto baseEntries = flasc::JsonMerge::SplitTopLevelObject(baseText);

        std::vector<std::string> duplicates;
        std::string merged = flasc::JsonMerge::Merge(baseEntries, insertEntries, duplicates);

        if (!duplicates.empty()) {
            std::cout << "Warning: " << duplicates.size()
                      << " key(s) exist in both files (both copies were kept):\n";
            for (const auto& key : duplicates)
                std::cout << "  " << key << "\n";
        }

        std::cout << "Merged " << insertEntries.size() << " entrie(s) into "
                  << baseEntries.size() << " existing entrie(s).\n";
        WriteOutputFile(exeDir, "merged.json", merged);
    } catch (const std::exception& ex) {
        std::cout << "Merge failed: " << ex.what() << "\n";
    }
}

// Resolves ivamFolder once per program run: uses FLASC's own folder if
// ivam.exe is sitting right there, and only asks the user otherwise. Once
// resolved (either way), it's reused for the rest of the session. ivam.exe
// only ever operates on its own current folder (it takes no file-path
// argument at all), so ivam.exe and everything it needs - Hashes.txt, the
// .dat files to process - all have to live together in that one folder.
bool EnsureIvamFolder(std::string& ivamFolder, const std::string& exeDir) {
    if (!ivamFolder.empty())
        return true;

    if (flasc::IvamRunner::ExistsIn(exeDir)) {
        ivamFolder = exeDir;
        return true;
    }

    std::cout << "ivam.exe not found next to FLASC. Path to folder with ivam.exe: ";
    std::string input;
    flasc::ReadLineUtf8(input);
    input = Trim(input);

    if (!input.empty() && flasc::IvamRunner::ExistsIn(input)) {
        ivamFolder = input;
        return true;
    }

    std::cout << "ivam.exe not found in that folder.\n";
    return false;
}

void ReportIvamResult(const flasc::IvamRunner::RunResult& result,
                      const std::vector<flasc::IvamRunner::ProcessedFile>& files,
                      const char* verb, const char* nothingFoundHint) {
    if (!result.started) {
        std::cout << "Could not start ivam.exe: " << result.error << "\n";
        return;
    }

    if (files.empty()) {
        std::cout << nothingFoundHint << "\n";
        return;
    }

    std::cout << verb << " files:\n";
    for (const auto& f : files) {
        if (f.outputExists)
            std::cout << "  " << f.inputName << " -> " << f.expectedOutputName << "\n";
        else
            std::cout << "  " << f.inputName << " -> FAILED (no " << f.expectedOutputName
                      << " appeared)\n";
    }
}

void ParseWithIvam(const std::string& exeDir, std::string& ivamFolder) {
    if (!EnsureIvamFolder(ivamFolder, exeDir))
        return;

    fs::path hashesPath = flasc::ToPath(ivamFolder) / "Hashes.txt";
    std::error_code ec;
    if (!fs::is_regular_file(hashesPath, ec)) {
        std::cout << "Warning: Hashes.txt not found next to ivam.exe - hash fields "
                     "will show as hex instead of names.\n";
    }

    auto result = flasc::IvamRunner::Run(ivamFolder, /*genMode=*/false);
    auto files = flasc::IvamRunner::CheckProcessed(ivamFolder, /*genMode=*/false);

    ReportIvamResult(result, files, "Parsed",
                     "No recognised audio metadata files found in the ivam.exe folder "
                     "(game.dat16, categories.dat15, etc).");
}

void BuildWithIvam(const std::string& exeDir, std::string& ivamFolder) {
    if (!EnsureIvamFolder(ivamFolder, exeDir))
        return;

    auto result = flasc::IvamRunner::Run(ivamFolder, /*genMode=*/true);
    auto files = flasc::IvamRunner::CheckProcessed(ivamFolder, /*genMode=*/true);

    ReportIvamResult(result, files, "Built",
                     "No recognised audio metadata files found in the ivam.exe folder "
                     "(game.dat16, categories.dat15, etc) - a build also needs the "
                     "matching .json sitting next to each one.");
}

} // namespace

#if defined(_WIN32)
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char* argv[]) {
#endif
    flasc::InitConsoleUtf8();

    std::string exeDir = GetExeDir();
    std::string ivamFolder; // resolved lazily, reused for the rest of the session

    // Optional: a folder (or ivam.exe itself) dragged onto FLASC.exe, or passed
    // via a shortcut/batch file. This arrives through argv with full Unicode
    // fidelity and no console I/O involved at all, so it works even for paths
    // where pasting into the interactive prompt below has proven unreliable.
    if (argc >= 2) {
#if defined(_WIN32)
        std::string arg = flasc::WideArgToUtf8(argv[1]);
#else
        std::string arg = argv[1];
#endif
        std::error_code ec;
        fs::path argPath = flasc::ToPath(arg);
        if (fs::is_directory(argPath, ec) && flasc::IvamRunner::ExistsIn(arg)) {
            ivamFolder = arg;
        } else if (fs::is_regular_file(argPath, ec)) {
            std::string parentDir = flasc::FromPath(argPath.parent_path());
            if (flasc::IvamRunner::ExistsIn(parentDir))
                ivamFolder = parentDir;
        }
        if (!ivamFolder.empty())
            std::cout << "Using ivam.exe folder from command line: " << ivamFolder << "\n";
    }

    while (true) {
        std::cout << "\nFLASC\n";
        std::cout << "1) Convert .ini to .json (FLA -> ivam)\n";
        std::cout << "2) Merge .json files\n";
        std::cout << "3) Parse files (ivam)\n";
        std::cout << "4) Build files (ivam)\n";
        std::cout << "5) Exit\n";
        std::cout << "> ";

        std::string choice;
        if (!flasc::ReadLineUtf8(choice))
            break;

        if (choice == "1") {
            ConvertIniToJson(exeDir);
        } else if (choice == "2") {
            MergeJsonFiles(exeDir);
        } else if (choice == "3") {
            ParseWithIvam(exeDir, ivamFolder);
        } else if (choice == "4") {
            BuildWithIvam(exeDir, ivamFolder);
        } else if (choice == "5") {
            break;
        } else {
            std::cout << "Unrecognised option.\n";
        }
    }

    return 0;
}
