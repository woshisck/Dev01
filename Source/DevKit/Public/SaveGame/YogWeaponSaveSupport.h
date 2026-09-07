#pragma once

#include "SaveGame/YogSaveGame.h"
#include "Engine/Blueprint.h"

namespace YogWeaponSaveSupport
{
inline void CaptureLayer(TSubclassOf<UYogAnimInstance> Layer, FWeaponInstanceData& Out)
{
	Out.WeaponLayer = Layer;
	Out.WeaponLayerClassPath = Layer ? Layer->GetPathName() : FString();
}

inline TSubclassOf<UYogAnimInstance> ResolveLayer(const FWeaponInstanceData& Data,
	TSubclassOf<UYogAnimInstance> DefaultLayer)
{
	if (!Data.WeaponLayerClassPath.IsEmpty())
	{
		UObject* SavedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *Data.WeaponLayerClassPath);
		UClass* LayerClass = Cast<UClass>(SavedObject);
		if (const UBlueprint* Blueprint = Cast<UBlueprint>(SavedObject)) LayerClass = Blueprint->GeneratedClass;
		if (LayerClass && LayerClass->IsChildOf(UYogAnimInstance::StaticClass())) return LayerClass;
		// Old versions saved UClass's meta-class path. Recover from the legacy field/CDO;
		// the per-instance SaveGame byte payload can still restore the original layer.
		return Data.WeaponLayer ? Data.WeaponLayer : DefaultLayer;
	}
	return Data.WeaponLayer;
}
}
