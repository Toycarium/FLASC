#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "IniLoader.h"
#include "Json.h"
#include "JsonMerge.h"
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
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    fs::path exePath(std::string(buffer, len));
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    fs::path exePath(len == -1 ? "." : std::string(buffer, static_cast<size_t>(len)));
#endif
    return exePath.parent_path().string();
}

std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n\"");
    size_t end = s.find_last_not_of(" \t\r\n\"");
    if (start == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
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
        candidates.push_back((fs::path(exeDir) / candidates[i]).string());

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (fs::is_regular_file(candidate, ec))
            return candidate;
    }
    return "";
}

std::string ReadWholeFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
        throw std::runtime_error("Could not open file: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void WriteOutputFile(const std::string& exeDir, const std::string& fileName,
                     const std::string& contents) {
    fs::path outputDir = fs::path(exeDir) / "output";
    fs::create_directories(outputDir);
    fs::path outputPath = outputDir / fileName;

    std::ofstream out(outputPath, std::ios::binary);
    out << contents;
    out.close();

    std::cout << "Written to: " << outputPath.string() << "\n";
}

void ConvertIniToJson(const std::string& exeDir) {
    std::cout << "Path to .ini file: ";
    std::string input;
    std::getline(std::cin, input);

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
    std::getline(std::cin, insertInput);
    std::string insertPath = ResolvePath(insertInput, exeDir, ".json");
    if (insertPath.empty()) {
        std::cout << "File not found.\n";
        return;
    }

    std::cout << "Path to .json file to merge into: ";
    std::string baseInput;
    std::getline(std::cin, baseInput);
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

} // namespace

int main() {
    std::string exeDir = GetExeDir();

    while (true) {
        std::cout << "\nFLASC\n";
        std::cout << "1) Convert .ini to .json (FLA -> ivam)\n";
        std::cout << "2) Merge .json files\n";
        std::cout << "3) Exit\n";
        std::cout << "> ";

        std::string choice;
        if (!std::getline(std::cin, choice))
            break;

        if (choice == "1") {
            ConvertIniToJson(exeDir);
        } else if (choice == "2") {
            MergeJsonFiles(exeDir);
        } else if (choice == "3") {
            break;
        } else {
            std::cout << "Unrecognised option.\n";
        }
    }

    return 0;
}
