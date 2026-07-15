#include "JsonMerge.h"

#include <cctype>
#include <set>
#include <stdexcept>
#include <unordered_set>

namespace flasc {
namespace JsonMerge {

namespace {

size_t SkipWhitespace(const std::string& text, size_t pos) {
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
        ++pos;
    return pos;
}

// pos must point at the opening quote. Returns the position just after the
// closing quote.
size_t SkipJsonString(const std::string& text, size_t pos) {
    size_t i = pos + 1;
    while (i < text.size()) {
        if (text[i] == '\\') {
            i += 2;
            continue;
        }
        if (text[i] == '"')
            return i + 1;
        ++i;
    }
    throw std::runtime_error("Unterminated JSON string");
}

// pos must point at the first character of the value (no leading whitespace).
// Returns the position just past the end of the value. Handles objects,
// arrays, strings, and bare tokens (numbers/true/false/null) via brace/
// bracket/string-aware scanning - it never needs to understand what the value
// actually *means*.
size_t SkipJsonValue(const std::string& text, size_t pos) {
    if (pos >= text.size())
        throw std::runtime_error("Unexpected end of JSON while reading a value");

    char c = text[pos];
    if (c == '{' || c == '[') {
        char open = c;
        char close = (c == '{') ? '}' : ']';
        int depth = 0;
        size_t i = pos;
        while (i < text.size()) {
            char ch = text[i];
            if (ch == '"') {
                i = SkipJsonString(text, i);
                continue;
            }
            if (ch == open) {
                ++depth;
            } else if (ch == close) {
                --depth;
                if (depth == 0)
                    return i + 1;
            }
            ++i;
        }
        throw std::runtime_error("Unterminated JSON object/array");
    }

    if (c == '"')
        return SkipJsonString(text, pos);

    // Bare token: number, true, false, null - scan until a structural delimiter.
    size_t i = pos;
    while (i < text.size() && text[i] != ',' && text[i] != '}' && text[i] != ']' &&
           !std::isspace(static_cast<unsigned char>(text[i]))) {
        ++i;
    }
    if (i == pos)
        throw std::runtime_error("Expected a JSON value");
    return i;
}

} // namespace

std::vector<RawEntry> SplitTopLevelObject(const std::string& text) {
    std::vector<RawEntry> entries;

    size_t i = SkipWhitespace(text, 0);
    if (i >= text.size() || text[i] != '{')
        throw std::runtime_error("Expected top-level JSON object ('{')");
    ++i;

    i = SkipWhitespace(text, i);
    if (i < text.size() && text[i] == '}')
        return entries; // empty object

    while (true) {
        i = SkipWhitespace(text, i);
        if (i >= text.size() || text[i] != '"')
            throw std::runtime_error("Expected a string key");

        size_t keyStart = i;
        size_t keyEnd = SkipJsonString(text, i); // position just past closing quote
        std::string rawKey = text.substr(keyStart + 1, (keyEnd - keyStart) - 2);
        i = keyEnd;

        i = SkipWhitespace(text, i);
        if (i >= text.size() || text[i] != ':')
            throw std::runtime_error("Expected ':' after key \"" + rawKey + "\"");
        ++i;

        i = SkipWhitespace(text, i);
        size_t valueStart = i;
        size_t valueEnd = SkipJsonValue(text, i);
        std::string rawValue = text.substr(valueStart, valueEnd - valueStart);

        entries.push_back({std::move(rawKey), std::move(rawValue)});

        i = SkipWhitespace(text, valueEnd);
        if (i < text.size() && text[i] == ',') {
            ++i;
            continue;
        }
        if (i < text.size() && text[i] == '}')
            break;
        throw std::runtime_error("Expected ',' or '}' after a value");
    }

    return entries;
}

std::string Merge(const std::vector<RawEntry>& base,
                   const std::vector<RawEntry>& inserted,
                   std::vector<std::string>& outDuplicateKeys) {
    outDuplicateKeys.clear();

    std::unordered_set<std::string> baseKeys;
    for (const auto& e : base)
        baseKeys.insert(e.key);
    for (const auto& e : inserted) {
        if (baseKeys.count(e.key))
            outDuplicateKeys.push_back(e.key);
    }

    std::vector<const RawEntry*> all;
    all.reserve(base.size() + inserted.size());
    for (const auto& e : base)
        all.push_back(&e);
    for (const auto& e : inserted)
        all.push_back(&e);

    if (all.empty())
        return "{}";

    std::string out = "{\n";
    for (size_t idx = 0; idx < all.size(); ++idx) {
        out += "\t\"";
        out += all[idx]->key;
        out += "\": ";
        out += all[idx]->rawValue;
        if (idx + 1 < all.size())
            out += ',';
        out += '\n';
    }
    out += "}";
    return out;
}

} // namespace JsonMerge
} // namespace flasc
