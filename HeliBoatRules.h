#pragma once

#include <optional>
#include <string>

#include "FieldSpec.h"

namespace flasc {
namespace HeliBoatRules {

inline constexpr const char* HeliTypeName = "gameHeli";
inline constexpr int HeliTypeId = 4;

inline constexpr const char* BoatTypeName = "gameBoat";
inline constexpr int BoatTypeId = 7;

struct Classification {
    FieldKind kind;
    ValueRule rule;
    std::string jsonKey;
};

// Classifies one raw ini key for a heli (structType 4) section.
// Returns std::nullopt if the key is a plain sound/hash name (caller should hash it as-is).
// Throws std::runtime_error for a "field_XX"/"floatAsInt_XX" key at an offset we haven't
// seen before, rather than silently guessing.
std::optional<Classification> ClassifyHeliField(const std::string& iniKey);

// Same as above, for a boat (structType 7) section.
std::optional<Classification> ClassifyBoatField(const std::string& iniKey);

} // namespace HeliBoatRules
} // namespace flasc
