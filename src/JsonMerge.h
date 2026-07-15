#pragma once

#include <string>
#include <vector>

namespace flasc {
namespace JsonMerge {

struct RawEntry {
    std::string key;
    std::string rawValue; // exact original JSON text for this value, untouched
};

// Splits a top-level JSON object "{ \"a\": ..., \"b\": ... }" into its immediate
// key/value pairs, without parsing *into* each value - each value is kept as
// the exact original substring of text. Throws std::runtime_error on malformed
// input (not a JSON object, unterminated string, mismatched braces, etc).
std::vector<RawEntry> SplitTopLevelObject(const std::string& text);

// Concatenates two already-split entry lists (base's entries first, then
// inserted's, matching "insert file's vehicles go at the bottom of the base
// file") into a single formatted JSON object string. Warns (via the returned
// duplicateKeys list) about any key that appears in both inputs, but still
// includes both copies - the caller decides what, if anything, to tell the user.
std::string Merge(const std::vector<RawEntry>& base,
                   const std::vector<RawEntry>& inserted,
                   std::vector<std::string>& outDuplicateKeys);

} // namespace JsonMerge
} // namespace flasc
