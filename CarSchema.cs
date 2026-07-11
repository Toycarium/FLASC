using System.Collections.Generic;

namespace FLASC
{
    /// <summary>
    /// gameAutomobile (structType 0) field layout.
    /// Verified field-for-field against GTAAudioMetadataTool_V2's SchemaRegistration_Game.cpp
    /// by cross-checking every ini field's embedded hex byte-offset (e.g. "field_11A" = offset 0x11A)
    /// against the schema's own computed byte offsets. Zero mismatches across all 91 ini fields / 90
    /// schema fields (the one discrepancy - field_15E + field_15F - is because those are really the
    /// two bytes of a single Int16 field, "GpsVoice", not two independent one-byte fields).
    /// </summary>
    public static class CarSchema
    {
        public const string TypeName = "gameAutomobile";
        public const int TypeId = 0;

        public static readonly IReadOnlyList<FieldSpec> Fields = new List<FieldSpec>
        {
            new FieldSpec("floatAsInt_A", "masterVolume", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_E", "MaxConeAttenuation", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("engineLowSound", "lowEngineLoop", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("engineHighSound", "highEngineLoop", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("exhaustLowSound", "lowExhaustLoop", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("exhaustHighSound", "highExhaustLoop", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("revsOffSound", "revsOffLoop", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("field_26", "EngineAccelVolume", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("field_2A", "ExhaustAccelVolume", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("floatAsInt_2E", "EngineAccelMinPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_32", "EngineAccelMaxPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("field_36", "EngineDecelVolume", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("field_3A", "ExhaustDecelVolume", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("floatAsInt_3E", "EngineDecelMinPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_42", "EngineDecelMaxPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("field_46", "EngineIdleVolume", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("field_4A", "ExhaustIdleVolume", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("floatAsInt_4E", "EngineIdleMinPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_52", "EngineIdleMaxPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("field_56", "EngineRevsVolume", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("field_5A", "ExhaustRevsVolume", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("floatAsInt_5E", "EngineRevsMinPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_62", "EngineRevsMaxPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_66", "ThrottleResonanceVolume", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("field_6A", "Filter1Cutoff", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("field_6E", "Filter2Cutoff", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("floatAsInt_72", "ResonanceMinPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_76", "ResonanceMaxPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("engineWaveShape", "EngineSynthDef", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("floatAsInt_7E", "EngineWaveShapePitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("exhaustWaveShape", "ExhaustSynthDef", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("floatAsInt_86", "ExhaustWaveShapePitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_multiplier1200_8A", "MinPitch", FieldKind.Int32, ValueRule.Scale1200),
            new FieldSpec("floatAsInt_multiplier1200_8E", "MaxPitch", FieldKind.Int32, ValueRule.Scale1200),
            new FieldSpec("engineIdleLoopSound", "engineIdleLoopSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("exhaustIdleLoopSound", "exhaustIdleLoopSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("floatAsInt_multiplier1200_9A", "IdleMinPitch", FieldKind.Int32, ValueRule.Scale1200),
            new FieldSpec("floatAsInt_multiplier1200_9E", "IdleMaxPitch", FieldKind.Int32, ValueRule.Scale1200),
            new FieldSpec("transmissionSound", "transmissionSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("floatAsInt_A6", "TransWhineMinPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_AA", "TransWhineMaxPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("sound_AE", "InductionLoop", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("floatAsInt_B2", "InductionMinPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_B6", "InductionMaxPitch", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("exhaustPopSound", "exhaustPopSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("airIntakeSound", "TurboWhine", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("floatAsInt_multiplier1200_C2", "TurboMinPitch", FieldKind.Int32, ValueRule.Scale1200),
            new FieldSpec("floatAsInt_multiplier1200_C6", "TurboMaxPitch", FieldKind.Int32, ValueRule.Scale1200),
            new FieldSpec("dumpValveSound", "dumpValveSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("startSound", "startupRevs", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("hornSound", "hornSounds", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("openSound", "doorOpenSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("closeSound", "doorCloseSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("trunkOpenSound", "bootOpenSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("trunkCloseSound", "bootCloseSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("field_E6", "BrakeSqueekFactor", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("suspensionUpSound", "suspensionUpSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("suspensionDownSound", "suspensionDownSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("field_F2", "minSuspCompThresh", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("field_F6", "maxSuspCompThresh", FieldKind.Float, ValueRule.Raw),
            new FieldSpec("policeScannerManufacturerSound", "policeScannerManufacturerSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("policeScannerModelSound", "policeScannerModelSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("policeScannerVehicleCategorySound", "policeScannerVehicleCategorySound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("gearTransmissionSound", "gearTransmissionSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("floatAsInt_multiplier1200_10A", "GearTransMinPitch", FieldKind.Int32, ValueRule.Scale1200),
            new FieldSpec("floatAsInt_multiplier1200_10E", "GearTransMaxPitch", FieldKind.Int32, ValueRule.Scale1200),
            new FieldSpec("floatAsInt_multiplier1200_112", "GTThrottleVol", FieldKind.Int32, ValueRule.Scale1200),
            new FieldSpec("floatAsInt_116", "DumpValveProb", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("field_11A", "TurboSpinUpSpeed", FieldKind.Int32, ValueRule.Raw),
            new FieldSpec("floatAsInt_11E", "VolumeBoost", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_122", "ExhaustBoost", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("floatAsInt_126", "TransmissionBoost", FieldKind.Int32, ValueRule.Scale100),
            new FieldSpec("jumpLandSound", "jumpLandSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("field_12E", "JumpLandMinThresh", FieldKind.Int32, ValueRule.Raw),
            new FieldSpec("field_132", "JumpLandMaxThresh", FieldKind.Int32, ValueRule.Raw),
            new FieldSpec("ignitionSound", "ignitionSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("engineShutDownSound", "engineShutDownSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("field_13E", "VolumeCategory", FieldKind.Int8, ValueRule.Raw),
            new FieldSpec("field_13F", "GPSType", FieldKind.Int8, ValueRule.Raw),
            new FieldSpec("field_140", "RadioType", FieldKind.Int8, ValueRule.Raw),
            new FieldSpec("field_141", "RadioGenre", FieldKind.Int8, ValueRule.Raw),
            new FieldSpec("indicatorVehicleOn", "indicatorOnSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("indicatorVehicleOff", "indicatorOffSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("engineCoolingFan", "coolingFanSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("handbrakeSound_14E", "handbrakeSound2", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("sound_152", "nullSound2", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("sound_156", "nullSound3", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("handbrakeSound_15A", "handbrakeSound", FieldKind.Hash, ValueRule.Raw),
            new FieldSpec("field_15E", "GpsVoice", FieldKind.Int16, ValueRule.PackedInt16Lo),
            new FieldSpec("field_15F", "GpsVoice", FieldKind.Int16, ValueRule.PackedInt16Hi),
            new FieldSpec("field_160", "RadioLeakage", FieldKind.Int8, ValueRule.Raw),
        };
    }
}
