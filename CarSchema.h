#pragma once

#include <vector>

#include "FieldSpec.h"

namespace flasc {

// gameAutomobile (structType 0) field layout.
// Verified field-for-field against GTAAudioMetadataTool_V2's SchemaRegistration_Game.cpp
// by cross-checking every ini field's embedded hex byte-offset (e.g. "field_11A" = offset
// 0x11A) against the schema's own computed byte offsets. Zero mismatches across all 91
// ini fields / 90 schema fields (the one discrepancy - field_15E + field_15F - is because
// those are really the two bytes of a single Int16 field, "GpsVoice", not two independent
// one-byte fields - confirmed directly against ivam's source: Int16("GpsVoice")).
namespace CarSchema {

inline constexpr const char* TypeName = "gameAutomobile";
inline constexpr int TypeId = 0;

const std::vector<FieldSpec>& Fields();

} // namespace CarSchema
} // namespace flasc
