using System;
using System.Globalization;
using FLASC.Json;

namespace FLASC
{
    public static class VehicleJsonBuilder
    {
        /// <summary>
        /// Converts one .ini section (e.g. [admiral]) into an ivam-compatible JSON object:
        /// { "Type": "...", "nametableOffset": N, "padding": 0, "Metadata": { ... } }
        ///
        /// Note on "nametableOffset": this is what the old community docs called "flags".
        /// We now know (cross-checked against a real decompiled game.dat16.json) that this
        /// value equals the *hex* number written in the old .ini's "flags" field - so for
        /// now we just carry that value through unchanged. What the field is actually used
        /// for by the engine, and what value a genuinely BRAND NEW (non-cloned) entry needs,
        /// is still unresolved - see conversation notes. Don't trust this value blindly for
        /// newly-created vehicles yet.
        /// </summary>
        public static JObject BuildVehicleObject(IniSection section)
        {
            string? structTypeStr = section.Get("structType");
            if (structTypeStr == null)
                throw new InvalidOperationException($"[{section.Name}] has no structType");

            int structType = int.Parse(structTypeStr, CultureInfo.InvariantCulture);

            string flagsStr = section.Get("flags") ?? "0x0";
            uint nametableOffset = ParseHex(flagsStr);

            var obj = new JObject();

            switch (structType)
            {
                case CarSchema.TypeId:
                    obj["Type"] = new JString(CarSchema.TypeName);
                    break;
                case HeliBoatRules.HeliTypeId:
                    obj["Type"] = new JString(HeliBoatRules.HeliTypeName);
                    break;
                case HeliBoatRules.BoatTypeId:
                    obj["Type"] = new JString(HeliBoatRules.BoatTypeName);
                    break;
                default:
                    throw new InvalidOperationException(
                        $"[{section.Name}] has unsupported structType {structType}");
            }

            obj["nametableOffset"] = new JInt(nametableOffset);
            obj["padding"] = new JInt(0);

            var metadata = new JObject();
            switch (structType)
            {
                case CarSchema.TypeId:
                    BuildCarMetadata(section, metadata);
                    break;
                case HeliBoatRules.HeliTypeId:
                    BuildHeliOrBoatMetadata(section, metadata, isBoat: false);
                    break;
                case HeliBoatRules.BoatTypeId:
                    BuildHeliOrBoatMetadata(section, metadata, isBoat: true);
                    break;
            }
            obj["Metadata"] = metadata;

            return obj;
        }

        private static void BuildCarMetadata(IniSection section, JObject metadata)
        {
            var fields = CarSchema.Fields;
            for (int i = 0; i < fields.Count; i++)
            {
                FieldSpec spec = fields[i];

                if (spec.Rule == ValueRule.PackedInt16Lo)
                {
                    // Next spec must be the matching Hi half of the same schema field.
                    FieldSpec hiSpec = fields[i + 1];
                    string loStr = section.Get(spec.IniKey) ?? "0";
                    string hiStr = section.Get(hiSpec.IniKey) ?? "0";
                    int lo = int.Parse(loStr, CultureInfo.InvariantCulture) & 0xFF;
                    int hi = int.Parse(hiStr, CultureInfo.InvariantCulture) & 0xFF;
                    short packed = unchecked((short)(lo | (hi << 8)));
                    metadata[spec.JsonName] = new JInt(packed);
                    i++; // skip the Hi half, already consumed
                    continue;
                }

                string? raw = section.Get(spec.IniKey);
                if (raw == null)
                    throw new InvalidOperationException(
                        $"[{section.Name}] missing expected field '{spec.IniKey}'");

                metadata[spec.JsonName] = ConvertValue(raw, spec.Kind, spec.Rule);
            }
        }

        private static void BuildHeliOrBoatMetadata(IniSection section, JObject metadata, bool isBoat)
        {
            foreach (var kv in section.Entries)
            {
                string key = kv.Key;
                if (key.Equals("structType", StringComparison.OrdinalIgnoreCase) ||
                    key.Equals("field_1", StringComparison.OrdinalIgnoreCase) ||
                    key.Equals("flags", StringComparison.OrdinalIgnoreCase))
                {
                    continue; // header fields, already handled
                }

                var classification = isBoat
                    ? HeliBoatRules.ClassifyBoatField(key)
                    : HeliBoatRules.ClassifyHeliField(key);

                if (classification == null)
                {
                    // Plain sound/hash field - keep its existing (already meaningful) name.
                    metadata[key] = new JString(JoaatHash.ToJsonHashString(kv.Value));
                    continue;
                }

                var (kind, rule, jsonKey) = classification.Value;
                metadata[jsonKey] = ConvertValue(kv.Value, kind, rule);
            }
        }

        private static JNode ConvertValue(string raw, FieldKind kind, ValueRule rule)
        {
            switch (kind)
            {
                case FieldKind.Hash:
                    return new JString(JoaatHash.ToJsonHashString(raw));

                case FieldKind.Flags4:
                    return new JInt(ParseHex(raw));

                case FieldKind.Float:
                    return new JFloat(double.Parse(raw, CultureInfo.InvariantCulture));

                case FieldKind.Int8:
                case FieldKind.UInt8:
                case FieldKind.Int16:
                case FieldKind.Int32:
                {
                    switch (rule)
                    {
                        case ValueRule.Scale100:
                            return new JInt((long)Math.Round(
                                double.Parse(raw, CultureInfo.InvariantCulture) * 100.0,
                                MidpointRounding.AwayFromZero));
                        case ValueRule.Scale1200:
                            return new JInt((long)Math.Round(
                                double.Parse(raw, CultureInfo.InvariantCulture) * 1200.0,
                                MidpointRounding.AwayFromZero));
                        default: // Raw
                            return new JInt(int.Parse(raw, CultureInfo.InvariantCulture));
                    }
                }

                default:
                    throw new InvalidOperationException($"Unhandled field kind {kind}");
            }
        }

        private static uint ParseHex(string text)
        {
            text = text.Trim();
            if (text.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
                text = text.Substring(2);
            return uint.Parse(text, NumberStyles.HexNumber, CultureInfo.InvariantCulture);
        }
    }
}
