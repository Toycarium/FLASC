#include "IniLoader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace flasc {

namespace {

std::string Trim(const std::string& s) {
    size_t start = 0;
    size_t end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start])))
        ++start;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;
    return s.substr(start, end - start);
}

bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}

} // namespace

std::optional<std::string> IniSection::Get(const std::string& key) const {
    for (const auto& kv : entries) {
        if (EqualsIgnoreCase(kv.first, key))
            return kv.second;
    }
    return std::nullopt;
}

namespace IniLoader {

std::vector<IniSection> Load(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Could not open file: " + path);

    std::vector<IniSection> sections;
    IniSection current;
    bool hasCurrent = false;

    auto flush = [&]() {
        if (hasCurrent) {
            sections.push_back(std::move(current));
            current = IniSection{};
        }
    };

    std::string rawLine;
    while (std::getline(file, rawLine)) {
        // getline splits on '\n' only, so a trailing '\r' from CRLF line endings
        // needs stripping too - Trim() already eats it since '\r' is whitespace.
        std::string line = Trim(rawLine);
        if (line.empty())
            continue;

        if (line.front() == '[' && line.back() == ']') {
            flush();
            current.name = Trim(line.substr(1, line.size() - 2));
            hasCurrent = true;
            continue;
        }

        if (!hasCurrent)
            continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = Trim(line.substr(0, eq));
        std::string value = Trim(line.substr(eq + 1));
        current.entries.emplace_back(std::move(key), std::move(value));
    }
    flush();

    return sections;
}

} // namespace IniLoader
} // namespace flasc
