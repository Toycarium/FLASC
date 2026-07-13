#include "VehicleJsonBuilder.h"

#include <cmath>
#include <stdexcept>

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

void BuildHeliOrBoatMetadata(const IniSection& section, json::JObject& metadata, bool isBoat) {
    for (const auto& kv : section.entries) {
        const std::string& key = kv.first;
        if (key == "structType" || key == "field_1" || key == "flags")
            continue; // header fields, already handled

        auto classification = isBoat ? HeliBoatRules::ClassifyBoatField(key)
                                      : HeliBoatRules::ClassifyHeliField(key);

        if (!classification.has_value()) {
            // Plain sound/hash field - keep its existing (already meaningful) name.
            metadata.Set(key, json::MakeString(JoaatHash::ToJsonHashString(kv.second)));
            continue;
        }

        metadata.Set(classification->jsonKey,
                     ConvertValue(kv.second, classification->kind, classification->rule));
    }
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
