#pragma once

#include <string>

namespace flasc {

// The true native storage type of a field inside a game.dat16 container, as
// confirmed against the GTAAudioMetadataTool_V2 (ivam) schema.
enum class FieldKind {
    Int8,
    UInt8,
    Int16,
    Int32,
    Float,
    Hash,    // stored as a 4-byte JOAAT hash, represented here as a string
    Flags4,  // stored as a raw 4-byte value parsed from a hex string (e.g. "0x2AA629")
};

// Describes how to turn the *textual* value from the old FLASC .ini format into
// the field's true native value. The old ini format expressed several fields in
// "human units" (e.g. a float that really needs to be multiplied by 100 or 1200
// and stored as a raw int) - this documents exactly which conversion applies,
// instead of guessing it from the field name at conversion time.
enum class ValueRule {
    Raw,            // ini text is already the native value (int, float, hex, or a sound name string)
    Scale100,       // ini float value * 100, rounded -> stored as int
    Scale1200,      // ini float value * 1200, rounded -> stored as int
    PackedInt16Lo,  // this ini field is the LOW byte of a 2-byte Int16 pair (e.g. old field_15E)
    PackedInt16Hi,  // this ini field is the HIGH byte of the same Int16 pair (e.g. old field_15F)
};

// One field of a fixed, fully-known schema (currently only gameAutomobile / structType 0).
struct FieldSpec {
    std::string iniKey;
    std::string jsonName;
    FieldKind kind;
    ValueRule rule;
};

} // namespace flasc
