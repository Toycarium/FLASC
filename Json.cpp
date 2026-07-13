#include "Json.h"

#include <charconv>
#include <cmath>
#include <stdexcept>

namespace flasc::json {

void JObject::Write(std::string& out, int indent) const {
    if (members_.empty()) {
        out += "{}";
        return;
    }

    out += "{\n";
    for (size_t i = 0; i < members_.size(); ++i) {
        WriteIndent(out, indent + 1);
        JString::WriteEscaped(out, members_[i].first);
        out += ": ";
        members_[i].second->Write(out, indent + 1);
        if (i + 1 < members_.size())
            out += ',';
        out += '\n';
    }
    WriteIndent(out, indent);
    out += '}';
}

void JString::WriteEscaped(std::string& out, const std::string& value) {
    out += '"';
    for (unsigned char c : value) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    static const char* hex = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(c >> 4) & 0xF];
                    out += hex[c & 0xF];
                } else {
                    out += static_cast<char>(c);
                }
                break;
        }
    }
    out += '"';
}

void JFloat::Write(std::string& out, int indent) const {
    (void)indent;

    if (std::isnan(value_) || std::isinf(value_))
        throw std::runtime_error("Cannot represent NaN/Infinity as JSON.");

    // std::to_chars for floating point gives the shortest decimal representation
    // that round-trips back to the exact same double - the same guarantee .NET's
    // default double.ToString() relies on. That means e.g. 640.0 -> "640" (no
    // decimal point), so we still need to force one on, same as the C# version.
    char buffer[64];
    auto result = std::to_chars(buffer, buffer + sizeof(buffer), value_);
    std::string text(buffer, result.ptr);

    if (text.find('.') == std::string::npos &&
        text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos) {
        text += ".0";
    }

    out += text;
}

} // namespace flasc::json
