#include "ActorFactories/ActorFactoryStylizedEmissivePreset.h"

#include "Actors/StylizedEmissiveLight.h"
#include "AssetRegistry/AssetData.h"
#include "StylizedEmissiveModelLibrary.h"

UActorFactoryStylizedEmissivePreset::UActorFactoryStylizedEmissivePreset()
{
	DisplayName = NSLOCTEXT("CelesLightEditor", "StylizedEmissivePresetFactoryName", "风格化自发光预设");
	NewActorClass = AStylizedEmissiveLight::StaticClass();
	bShowInEditorQuickMenu = false;
	bShouldAutoRegister = false;
}

void UActorFactoryStylizedEmissivePreset::Configure(UStylizedEmissiveModelLibrary* InLibrary, const FName InPresetId)
{
	Library = InLibrary;
	PresetId = InPresetId;
}

bool UActorFactoryStylizedEmissivePreset::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	UStylizedEmissiveModelLibrary* AssetLibrary = Cast<UStylizedEmissiveModelLibrary>(AssetData.GetAsset());
	if (!AssetLibrary || AssetLibrary != Library || !Library->FindModel(PresetId))
	{
		OutErrorMsg = NSLOCTEXT("CelesLightEditor", "InvalidStylizedEmissivePreset", "无效或已经删除的风格化自发光预设。");
		return false;
	}

	return true;
}

void UActorFactoryStylizedEmissivePreset::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	AStylizedEmissiveLight* EmissiveActor = Cast<AStylizedEmissiveLight>(NewActor);
	UStylizedEmissiveModelLibrary* AssetLibrary = Cast<UStylizedEmissiveModelLibrary>(Asset);
	if (EmissiveActor && AssetLibrary && EmissiveActor->ApplyLibraryPreset(AssetLibrary, PresetId))
	{
		EmissiveActor->SetActorLabel(GetDefaultActorLabel(AssetLibrary));
	}
}

UObject* UActorFactoryStylizedEmissivePreset::GetAssetFromActorInstance(AActor* ActorInstance)
{
	const AStylizedEmissiveLight* EmissiveActor = Cast<AStylizedEmissiveLight>(ActorInstance);
	return EmissiveActor ? EmissiveActor->ModelLibrary.Get() : nullptr;
}

FString UActorFactoryStylizedEmissivePreset::GetDefaultActorLabel(UObject* Asset) const
{
	if (const UStylizedEmissiveModelLibrary* AssetLibrary = Cast<UStylizedEmissiveModelLibrary>(Asset))
	{
		if (const FStylizedEmissiveModelEntry* Entry = AssetLibrary->FindModel(PresetId))
		{
			const FString Label = Entry->DisplayName.IsEmpty() ? Entry->ModelId.ToString() : Entry->DisplayName.ToString();
			return FString::Printf(TEXT("Emissive_%s"), *Label);
		}
	}

	return TEXT("StylizedEmissiveSource");
}
