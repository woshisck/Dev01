#pragma once

#include "ActorFactories/ActorFactory.h"
#include "ActorFactoryStylizedEmissivePreset.generated.h"

class UStylizedEmissiveModelLibrary;

/** Places one configured entry from the Stylized Emissive Library into a level viewport. */
UCLASS(Transient)
class CELESLIGHTEDITOR_API UActorFactoryStylizedEmissivePreset : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryStylizedEmissivePreset();

	void Configure(UStylizedEmissiveModelLibrary* InLibrary, FName InPresetId);

	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;

protected:
	virtual FString GetDefaultActorLabel(UObject* Asset) const override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UStylizedEmissiveModelLibrary> Library = nullptr;

	UPROPERTY(Transient)
	FName PresetId = NAME_None;
};
