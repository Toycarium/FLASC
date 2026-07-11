using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace FLASC
{
    /// <summary>
    /// gameHeli (structType 4) and gameBoat (structType 7) don't have a known field
    /// schema yet - ivam itself only treats them as an opaque Placeholder blob. Until
    /// someone reverse-engineers proper field names for these two types (the way we
    /// now have for gameAutomobile), we fall back to the byte-offset-based rules FLASC
    /// already established from the .ini, verified here against real "annihilator" /
    /// "dinghy" ini sections.
    ///
    /// Output JSON keys are normalised to "field_XX" (just the hex struct offset),
    /// dropping the old "floatAsInt_"/"floatAsInt_multiplier1200_" prefixes, since those
    /// prefixes described the *ini's* encoding, not anything meaningful about the field
    /// itself - once converted, everything is just a typed value at a given offset.
    /// Fields that already have a real sound/hash name keep that name unchanged.
    /// </summary>
    public static class HeliBoatRules
    {
        public const string HeliTypeName = "gameHeli";
        public const int HeliTypeId = 4;

        public const string BoatTypeName = "gameBoat";
        public const int BoatTypeId = 7;

        // structType 4 (helicopter) - offsets confirmed against the "annihilator" ini section
        private static readonly HashSet<string> HeliInt16Offsets = new HashSet<string>
        {
            "46", "48", "4C", "4E", "52", "54", "90", "92", "96", "98"
        };
        private static readonly HashSet<string> HeliFloatOffsets = new HashSet<string>
        {
            "1E", "58", "5C", "64", "68"
        };
        private static readonly HashSet<string> HeliByteOffsets = new HashSet<string>
        {
            "A8", "A9"
        };

        // structType 7 (boat) - offsets confirmed against the "dinghy" ini section
        private static readonly HashSet<string> BoatFloatOffsets = new HashSet<string>
        {
            "46", "4A", "56", "5A", "66", "6A", "76", "7A", "8A", "8E"
        };
        private static readonly HashSet<string> BoatRawIntOffsets = new HashSet<string>
        {
            "9E", "A6"
        };
        private static readonly HashSet<string> BoatByteOffsets = new HashSet<string>
        {
            "B6", "B7", "C8"
        };
        private const string BoatFlagsOffset = "42";

        private static readonly Regex OffsetSuffix =
            new Regex(@"^(?:field_|floatAsInt_multiplier1200_|floatAsInt_)([0-9A-Fa-f]+)$", RegexOptions.Compiled);

        /// <summary>
        /// Classifies one raw ini key for a heli (structType 4) section.
        /// Returns null if the key is a plain sound/hash name (caller should hash it as-is).
        /// </summary>
        public static (FieldKind kind, ValueRule rule, string jsonKey)? ClassifyHeliField(string iniKey)
        {
            var m = OffsetSuffix.Match(iniKey);
            if (!m.Success)
                return null; // plain sound name field, e.g. "rotorSound"

            string offset = m.Groups[1].Value.ToUpperInvariant();
            string jsonKey = "field_" + offset;

            if (HeliByteOffsets.Contains(offset))
                return (FieldKind.Int8, ValueRule.Raw, jsonKey);

            if (HeliFloatOffsets.Contains(offset))
                return (FieldKind.Float, ValueRule.Raw, jsonKey);

            if (iniKey.StartsWith("floatAsInt_", StringComparison.Ordinal))
                return (FieldKind.Int16, ValueRule.Scale100, jsonKey);

            if (HeliInt16Offsets.Contains(offset))
                return (FieldKind.Int16, ValueRule.Raw, jsonKey);

            // Fallback: any other "field_XX" we haven't explicitly classified.
            // Conservatively treat as raw Int16 (matches every currently-known case);
            // flagged loudly so a genuinely new offset doesn't silently misconvert.
            throw new InvalidOperationException(
                $"Unclassified heli field '{iniKey}' (offset 0x{offset}) - add it to HeliBoatRules before converting.");
        }

        /// <summary>
        /// Classifies one raw ini key for a boat (structType 7) section.
        /// Returns null if the key is a plain sound/hash name (caller should hash it as-is).
        /// </summary>
        public static (FieldKind kind, ValueRule rule, string jsonKey)? ClassifyBoatField(string iniKey)
        {
            var m = OffsetSuffix.Match(iniKey);
            if (!m.Success)
                return null; // plain sound name field, e.g. "engineLoopSound"

            string offset = m.Groups[1].Value.ToUpperInvariant();
            string jsonKey = "field_" + offset;

            if (offset == BoatFlagsOffset)
                return (FieldKind.Flags4, ValueRule.Raw, jsonKey);

            if (BoatByteOffsets.Contains(offset))
                return (FieldKind.Int8, ValueRule.Raw, jsonKey);

            if (BoatFloatOffsets.Contains(offset))
                return (FieldKind.Float, ValueRule.Raw, jsonKey);

            if (BoatRawIntOffsets.Contains(offset))
                return (FieldKind.Int32, ValueRule.Raw, jsonKey);

            if (iniKey.StartsWith("floatAsInt_", StringComparison.Ordinal))
                return (FieldKind.Int32, ValueRule.Scale100, jsonKey);

            throw new InvalidOperationException(
                $"Unclassified boat field '{iniKey}' (offset 0x{offset}) - add it to HeliBoatRules before converting.");
        }
    }
}
