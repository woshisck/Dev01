#include "Surface/DevKitDecalAsset.h"

#include "Materials/MaterialInterface.h"
#include "UObject/Package.h"

UDevKitDecalAsset::UDevKitDecalAsset()
	: DefaultTransform(FTransform::Identity)
{
	// Do not put a generated GUID on the CDO: it would be copied to every new asset.
	DefinitionGuid = FGuid();
}

void UDevKitDecalAsset::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) && !DefinitionGuid.IsValid())
	{
		DefinitionGuid = FGuid::NewGuid();
	}
}

void UDevKitDecalAsset::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);
	DefinitionGuid = FGuid::NewGuid();
}

bool UDevKitDecalAsset::IsValidDefinition() const
{
	if (!DefinitionGuid.IsValid() || Backend == EDevKitDecalBackend::Unsupported)
	{
		return false;
	}
	if (Backend == EDevKitDecalBackend::RVTPlane || Backend == EDevKitDecalBackend::RVTVisibleMesh)
	{
		return LegacyRVTAsset != nullptr || (Mesh != nullptr && Material != nullptr);
	}
	if (Backend == EDevKitDecalBackend::DeferredProjection)
	{
		return Material != nullptr;
	}
	return Mesh != nullptr && Material != nullptr;
}

bool UDevKitDecalAsset::IsRuntimeAddressable() const
{
	for (const FDevKitDecalRepresentation& Representation : Representations)
	{
		if (Representation.bRuntimeSwitchable)
		{
			return true;
		}
	}
	return UsageTags.HasTag(FGameplayTag::RequestGameplayTag(TEXT("DevKit.Decal.Runtime"), false));
}

FPrimaryAssetId UDevKitDecalAsset::GetPrimaryAssetId() const
{
	static const FPrimaryAssetType Type(TEXT("DevKitDecalAsset"));
	return FPrimaryAssetId(Type, GetFName());
}
