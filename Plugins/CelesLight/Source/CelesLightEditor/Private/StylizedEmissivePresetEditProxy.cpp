#include "StylizedEmissivePresetEditProxy.h"

void UStylizedEmissivePresetEditProxy::LoadFromEntry(const FStylizedEmissiveModelEntry& Entry)
{
	PresetId = Entry.ModelId;
	DisplayName = Entry.DisplayName;
	Description = Entry.Description;
	bUseMesh = Entry.bUseMesh;
	Mesh = Entry.Mesh;
	EmissiveMaterial = Entry.Material;
	RelativeTransform = Entry.RelativeTransform;
	LightingOutput = Entry.LightingOutput;
	LightColor = Entry.LightColor;
	Intensity = Entry.Intensity;
	EmissiveIntensity = Entry.EmissiveIntensity;
	AttenuationRadius = Entry.AttenuationRadius;
	bFillLight = Entry.bFillLight;
	SmoothStepMin = Entry.SmoothStepMin;
	SmoothStepMax = Entry.SmoothStepMax;
	SpecularOffset = Entry.SpecularOffset;
	EffectType = Entry.EffectType;
}

void UStylizedEmissivePresetEditProxy::WriteToEntry(FStylizedEmissiveModelEntry& Entry) const
{
	Entry.ModelId = PresetId;
	Entry.DisplayName = DisplayName;
	Entry.Description = Description;
	Entry.bUseMesh = bUseMesh;
	Entry.Mesh = Mesh;
	Entry.Material = EmissiveMaterial;
	Entry.RelativeTransform = RelativeTransform;
	Entry.LightingOutput = LightingOutput;
	Entry.LightColor = LightColor;
	Entry.Intensity = Intensity;
	Entry.EmissiveIntensity = EmissiveIntensity;
	Entry.AttenuationRadius = AttenuationRadius;
	Entry.bFillLight = bFillLight;
	Entry.SmoothStepMin = SmoothStepMin;
	Entry.SmoothStepMax = SmoothStepMax;
	Entry.SpecularOffset = SpecularOffset;
	Entry.EffectType = EffectType;
}
