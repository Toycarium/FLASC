#pragma once

#include <memory>

#include "IniLoader.h"
#include "Json.h"

namespace flasc {

namespace VehicleJsonBuilder {

// Converts one .ini section (e.g. [admiral]) into an ivam-compatible JSON object:
// { "Type": "...", "nametableOffset": N, "padding": 0, "Metadata": { ... } }
//
// Note on "nametableOffset": this is what the old community docs called "flags".
// We now know (cross-checked against a real decompiled game.dat16.json) that this
// value equals the *hex* number written in the old .ini's "flags" field - so for
// now we just carry that value through unchanged. What the field is actually used
// for by the engine, and what value a genuinely BRAND NEW (non-cloned) entry needs,
// is still unresolved. Don't trust this value blindly for newly-created vehicles yet.
std::unique_ptr<json::JObject> BuildVehicleObject(const IniSection& section);

} // namespace VehicleJsonBuilder
} // namespace flasc
