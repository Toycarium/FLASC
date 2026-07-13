#pragma once

#include <cstdint>
#include <string>

namespace flasc {

// Jenkins one-at-a-time hash ("joaat"), as used throughout GTA IV's RAGE audio
// metadata (identical to the algorithm in ivam's HashManager.cpp). Strings are
// always lower-cased before hashing.
namespace JoaatHash {

uint32_t Compute(const std::string& str);

// Produces the same textual representation ivam's JSON uses for a hash field:
// the lower-cased sound name if we have one, or "0x00000000" for the FLASC
// "NULL" sentinel (meaning "write 4 zero bytes", i.e. hash == 0 - NOT the joaat
// hash of the literal word "NULL").
std::string ToJsonHashString(const std::string& soundName);

} // namespace JoaatHash
} // namespace flasc
