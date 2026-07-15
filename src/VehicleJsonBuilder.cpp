#include "VehicleJsonBuilder.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "CarSchema.h"
#include "HeliBoatRules.h"
#include "JoaatHash.h"

namespace flasc {
namespace {

uint32_t ParseHex(std::string text) {
    // trim
    size_t start = text.find_first_not_of(" \t\r\n");
    size_t end = text.find_last_not_of(" \t\r\n");
    text = (start == std::string::npos) ? "" : text.substr(start, end - start + 1);

    if (text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
        text = text.substr(2);

    return static_cast<uint32_t>(std::stoul(text, nullptr, 16));
}

std::unique_ptr<json::JNode> ConvertValue(const std::string& raw, FieldKind kind, ValueRule rule) {
    switch (kind) {
        case FieldKind::Hash:
            return json::MakeString(JoaatHash::ToJsonHashString(raw));

        case FieldKind::Flags4:
            return json::MakeInt(ParseHex(raw));

        case FieldKind::Float:
            return json::MakeFloat(std::stod(raw));

        case FieldKind::Int8:
        case FieldKind::UInt8:
        case FieldKind::Int16:
        case FieldKind::Int32: {
            switch (rule) {
                case ValueRule::Scale100:
                    return json::MakeInt(static_cast<long long>(std::llround(std::stod(raw) * 100.0)));
                case ValueRule::Scale1200:
                    return json::MakeInt(static_cast<long long>(std::llround(std::stod(raw) * 1200.0)));
                default: // Raw
                    return json::MakeInt(std::stol(raw));
            }
        }

        default:
            throw std::runtime_error("Unhandled field kind");
    }
}

void BuildCarMetadata(const IniSection& section, json::JObject& metadata) {
    const auto& fields = CarSchema::Fields();
    for (size_t i = 0; i < fields.size(); ++i) {
        const FieldSpec& spec = fields[i];

        if (spec.rule == ValueRule::PackedInt16Lo) {
            // Next spec must be the matching Hi half of the same schema field.
            const FieldSpec& hiSpec = fields[i + 1];
            std::string loStr = section.Get(spec.iniKey).value_or("0");
            std::string hiStr = section.Get(hiSpec.iniKey).value_or("0");
            int lo = std::stoi(loStr) & 0xFF;
            int hi = std::stoi(hiStr) & 0xFF;
            int16_t packed = static_cast<int16_t>(lo | (hi << 8));
            metadata.Set(spec.jsonName, json::MakeInt(packed));
            ++i; // skip the Hi half, already consumed
            continue;
        }

        auto rawOpt = section.Get(spec.iniKey);
        if (!rawOpt.has_value()) {
            throw std::runtime_error("[" + section.name + "] missing expected field '" +
                                      spec.iniKey + "'");
        }

        metadata.Set(spec.jsonName, ConvertValue(*rawOpt, spec.kind, spec.rule));
    }
}

// Appends the little-endian byte representation of one field's value to buf,
// using the exact same width/conversion rules ConvertValue() uses for JSON -
// just written as raw bytes instead of a named JSON field. This is what lets
// gameHeli/gameBoat round-trip through the same opaque "Data" hex-blob format
// ivam itself uses for these two (still-unnamed) types.
void AppendFieldBytes(std::vector<uint8_t>& buf, FieldKind kind, ValueRule rule,
                       const std::string& raw) {
    auto pushLE = [&buf](uint32_t v, int numBytes) {
        for (int i = 0; i < numBytes; ++i)
            buf.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    };

    switch (kind) {
        case FieldKind::Hash:
            pushLE(JoaatHash::ComputeFieldHash(raw), 4);
            break;

        case FieldKind::Flags4:
            pushLE(ParseHex(raw), 4);
            break;

        case FieldKind::Float: {
            float f = static_cast<float>(std::stod(raw));
            uint32_t bits;
            std::memcpy(&bits, &f, sizeof(bits));
            pushLE(bits, 4);
            break;
        }

        case FieldKind::Int8:
        case FieldKind::UInt8:
            pushLE(static_cast<uint32_t>(std::stol(raw)) & 0xFF, 1);
            break;

        case FieldKind::Int16: {
            long long v;
            if (rule == ValueRule::Scale100)
                v = std::llround(std::stod(raw) * 100.0);
            else if (rule == ValueRule::Scale1200)
                v = std::llround(std::stod(raw) * 1200.0);
            else
                v = std::stol(raw);
            pushLE(static_cast<uint16_t>(static_cast<int16_t>(v)), 2);
            break;
        }

        case FieldKind::Int32: {
            long long v;
            if (rule == ValueRule::Scale100)
                v = std::llround(std::stod(raw) * 100.0);
            else if (rule == ValueRule::Scale1200)
                v = std::llround(std::stod(raw) * 1200.0);
            else
                v = std::stol(raw);
            pushLE(static_cast<uint32_t>(static_cast<int32_t>(v)), 4);
            break;
        }
    }
}

std::string BytesToHexString(const std::vector<uint8_t>& bytes) {
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 3);
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0)
            out += ' ';
        out += hex[(bytes[i] >> 4) & 0xF];
        out += hex[bytes[i] & 0xF];
    }
    return out;
}

// gameHeli / gameBoat don't have a known field schema in ivam either - it only
// stores them as an opaque byte blob ("Data"). Rather than inventing our own
// named-field JSON (which nothing else could read), we reconstruct the exact
// original binary payload - using the same offset/width rules HeliBoatRules
// already established - and expose it the same way ivam does, for maximum
// compatibility with the wider toolchain.
void BuildHeliOrBoatMetadata(const IniSection& section, json::JObject& metadata, bool isBoat) {
    std::vector<uint8_t> bytes;
    for (const auto& kv : section.entries) {
        const std::string& key = kv.first;
        if (key == "structType" || key == "field_1" || key == "flags")
            continue; // header fields, already handled

        if (isBoat && key == "field_C8")
            continue; // leftover artifact from the original .ini decode - always 0 across
                       // every vanilla boat, and one byte past the true 190-byte struct
                       // size ivam itself uses. Not a real field.

        auto classification = isBoat ? HeliBoatRules::ClassifyBoatField(key)
                                      : HeliBoatRules::ClassifyHeliField(key);

        if (!classification.has_value()) {
            // Plain sound/hash field.
            AppendFieldBytes(bytes, FieldKind::Hash, ValueRule::Raw, kv.second);
            continue;
        }

        AppendFieldBytes(bytes, classification->kind, classification->rule, kv.second);
    }

    metadata.Set("Data", json::MakeString(BytesToHexString(bytes)));
}

} // namespace

namespace VehicleJsonBuilder {

std::unique_ptr<json::JObject> BuildVehicleObject(const IniSection& section) {
    auto structTypeStr = section.Get("structType");
    if (!structTypeStr.has_value())
        throw std::runtime_error("[" + section.name + "] has no structType");

    int structType = std::stoi(*structTypeStr);

    std::string flagsStr = section.Get("flags").value_or("0x0");
    uint32_t nametableOffset = ParseHex(flagsStr);

    auto obj = std::make_unique<json::JObject>();

    if (structType == CarSchema::TypeId) {
        obj->Set("Type", json::MakeString(CarSchema::TypeName));
    } else if (structType == HeliBoatRules::HeliTypeId) {
        obj->Set("Type", json::MakeString(HeliBoatRules::HeliTypeName));
    } else if (structType == HeliBoatRules::BoatTypeId) {
        obj->Set("Type", json::MakeString(HeliBoatRules::BoatTypeName));
    } else {
        throw std::runtime_error("[" + section.name + "] has unsupported structType " +
                                  std::to_string(structType));
    }

    obj->Set("nametableOffset", json::MakeInt(nametableOffset));
    obj->Set("padding", json::MakeInt(0));

    auto metadata = std::make_unique<json::JObject>();
    if (structType == CarSchema::TypeId) {
        BuildCarMetadata(section, *metadata);
    } else if (structType == HeliBoatRules::HeliTypeId) {
        BuildHeliOrBoatMetadata(section, *metadata, /*isBoat=*/false);
    } else if (structType == HeliBoatRules::BoatTypeId) {
        BuildHeliOrBoatMetadata(section, *metadata, /*isBoat=*/true);
    }
    obj->Set("Metadata", std::move(metadata));

    return obj;
}

} // namespace VehicleJsonBuilder
} // namespace flasc
