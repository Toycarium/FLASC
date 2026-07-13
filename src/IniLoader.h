#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace flasc {

struct IniSection {
    std::string name;
    std::vector<std::pair<std::string, std::string>> entries;

    std::optional<std::string> Get(const std::string& key) const;
};

namespace IniLoader {

// Parses the whole .ini file into an ordered list of sections, each with an
// ordered list of key/value pairs (order matters: for gameAutomobile/gameHeli/
// gameBoat, field order in the .ini is the same as the field's byte offset
// order in the binary struct). Throws std::runtime_error if the file can't be
// opened.
std::vector<IniSection> Load(const std::string& path);

} // namespace IniLoader
} // namespace flasc
