#include "YogVisionSettings.h"

namespace
{
float SafeFloat(float Value, float Default, float Min, float Max)
{
    return FMath::IsFinite(Value) ? FMath::Clamp(Value, Min, Max) : Default;
}
FLinearColor SafeColor(const FLinearColor& Value, const FLinearColor& Default)
{
    return FLinearColor(SafeFloat(Value.R, Default.R, 0.f, 10.f), SafeFloat(Value.G, Default.G, 0.f, 10.f),
        SafeFloat(Value.B, Default.B, 0.f, 10.f), 1.f);
}
}

void FYogVisionParameters::Sanitize()
{
    const FYogVisionParameters Default;
    BaseSightRadius = SafeFloat(BaseSightRadius, Default.BaseSightRadius, 0.f, 20000.f);
    SpiritRadius = SafeFloat(SpiritRadius, Default.SpiritRadius, 0.f, 20000.f);
    SpiritStrength = SafeFloat(SpiritStrength, Default.SpiritStrength, 0.f, 1.f);
    SpiritDisturbance = SafeFloat(SpiritDisturbance, Default.SpiritDisturbance, 0.f, 3.f);
    SpiritVolumeHeight = SafeFloat(SpiritVolumeHeight, Default.SpiritVolumeHeight, 10.f, 2000.f);
    SpiritTrailLength = SafeFloat(SpiritTrailLength, Default.SpiritTrailLength, 0.f, 4000.f);
    FogDensity = SafeFloat(FogDensity, Default.FogDensity, 0.f, 1.f);
    FogContrast = SafeFloat(FogContrast, Default.FogContrast, 0.f, 1.f);
    FogBrightness = SafeFloat(FogBrightness, Default.FogBrightness, 0.f, 2.f);
    DistanceFogStart = SafeFloat(DistanceFogStart, Default.DistanceFogStart, 0.f, 50000.f);
    DistanceFogEnd = FMath::Max(DistanceFogStart + 1.f, SafeFloat(DistanceFogEnd, Default.DistanceFogEnd, 1.f, 100000.f));
    DistanceFogDensity = SafeFloat(DistanceFogDensity, Default.DistanceFogDensity, 0.f, 1.f);
    DarkThreshold = SafeFloat(DarkThreshold, Default.DarkThreshold, 0.f, 1.f);
    DarkFogStrength = SafeFloat(DarkFogStrength, Default.DarkFogStrength, 0.f, 1.f);
    EdgeStrength = SafeFloat(EdgeStrength, Default.EdgeStrength, 0.f, 2.f);
    StrokeStrength = SafeFloat(StrokeStrength, Default.StrokeStrength, 0.f, 1.f);
    StrokeScale = SafeFloat(StrokeScale, Default.StrokeScale, .25f, 8.f);
    FlowSpeed = SafeFloat(FlowSpeed, Default.FlowSpeed, 0.f, 10.f);
    BoundaryWidth = SafeFloat(BoundaryWidth, Default.BoundaryWidth, 1.f, 5000.f);
    BoundaryNoise = SafeFloat(BoundaryNoise, Default.BoundaryNoise, 0.f, 1.f);
    PlayerLift = SafeFloat(PlayerLift, Default.PlayerLift, 0.f, 1.f);
    FogColor = SafeColor(FogColor, Default.FogColor);
    SpiritColor = SafeColor(SpiritColor, Default.SpiritColor);
}

UYogVisionSettings::UYogVisionSettings()
{
    PostProcessMaterial = FSoftObjectPath(TEXT("/YogVision/Materials/M_VisionFog.M_VisionFog"));
}
