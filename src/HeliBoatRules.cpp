#include "HeliBoatRules.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>

namespace flasc {
namespace HeliBoatRules {

namespace {

// structType 4 (helicopter) - offsets confirmed against the "annihilator" ini section
const std::set<std::string> kHeliInt16Offsets = {"46", "48", "4C", "4E", "52", "54", "90", "92", "96", "98"};
const std::set<std::string> kHeliFloatOffsets = {"1E", "58", "5C", "64", "68"};
const std::set<std::string> kHeliByteOffsets = {"A8", "A9"};

// structType 7 (boat) - offsets confirmed against the "dinghy" ini section
const std::set<std::string> kBoatFloatOffsets = {"46", "4A", "56", "5A", "66", "6A", "76", "7A", "8A", "8E"};
const std::set<std::string> kBoatRawIntOffsets = {"9E", "A6"};
const std::set<std::string> kBoatByteOffsets = {"B6", "B7"};
const std::string kBoatFlagsOffset = "42";

bool IsHexDigits(const std::string& s) {
    if (s.empty())
        return false;
    return std::all_of(s.begin(), s.end(),
                        [](unsigned char c) { return std::isxdigit(c) != 0; });
}

std::string ToUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

bool StartsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

// Extracts the hex offset embedded in a "field_XX" / "floatAsInt_XX" /
// "floatAsInt_multiplier1200_XX" style key. Returns std::nullopt if the key
// doesn't match one of those three prefixes at all (i.e. it's a plain sound
// name field like "rotorSound").
std::optional<std::string> ExtractOffset(const std::string& iniKey) {
    static const std::string prefixes[] = {
        "floatAsInt_multiplier1200_", "floatAsInt_", "field_"};

    for (const auto& prefix : prefixes) {
        if (StartsWith(iniKey, prefix)) {
            std::string rest = iniKey.substr(prefix.size());
            if (IsHexDigits(rest))
                return ToUpper(rest);
        }
    }
    return std::nullopt;
}

} // namespace

std::optional<Classification> ClassifyHeliField(const std::string& iniKey) {
    auto offsetOpt = ExtractOffset(iniKey);
    if (!offsetOpt.has_value())
        return std::nullopt; // plain sound name field, e.g. "rotorSound"

    const std::string& offset = *offsetOpt;
    std::string jsonKey = "field_" + offset;

    if (kHeliByteOffsets.count(offset))
        return Classification{FieldKind::Int8, ValueRule::Raw, jsonKey};

    if (kHeliFloatOffsets.count(offset))
        return Classification{FieldKind::Float, ValueRule::Raw, jsonKey};

    if (StartsWith(iniKey, "floatAsInt_"))
        return Classification{FieldKind::Int16, ValueRule::Scale100, jsonKey};

    if (kHeliInt16Offsets.count(offset))
        return Classification{FieldKind::Int16, ValueRule::Raw, jsonKey};

    throw std::runtime_error(
        "Unclassified heli field '" + iniKey + "' (offset 0x" + offset +
        ") - add it to HeliBoatRules before converting.");
}

std::optional<Classification> ClassifyBoatField(const std::string& iniKey) {
    auto offsetOpt = ExtractOffset(iniKey);
    if (!offsetOpt.has_value())
        return std::nullopt; // plain sound name field, e.g. "engineLoopSound"

    const std::string& offset = *offsetOpt;
    std::string jsonKey = "field_" + offset;

    if (offset == kBoatFlagsOffset)
        return Classification{FieldKind::Flags4, ValueRule::Raw, jsonKey};

    if (kBoatByteOffsets.count(offset))
        return Classification{FieldKind::Int8, ValueRule::Raw, jsonKey};

    if (kBoatFloatOffsets.count(offset))
        return Classification{FieldKind::Float, ValueRule::Raw, jsonKey};

    if (kBoatRawIntOffsets.count(offset))
        return Classification{FieldKind::Int32, ValueRule::Raw, jsonKey};

    if (StartsWith(iniKey, "floatAsInt_"))
        return Classification{FieldKind::Int32, ValueRule::Scale100, jsonKey};

    throw std::runtime_error(
        "Unclassified boat field '" + iniKey + "' (offset 0x" + offset +
        ") - add it to HeliBoatRules before converting.");
}

} // namespace HeliBoatRules
} // namespace flasc
