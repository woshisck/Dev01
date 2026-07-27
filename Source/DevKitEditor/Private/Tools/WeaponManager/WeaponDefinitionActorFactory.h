#pragma once

#include "ActorFactories/ActorFactory.h"

#include "WeaponDefinitionActorFactory.generated.h"

class UWeaponDefinition;

/** Places a WeaponDefinition by spawning the shared BP_WeaponSpawner and assigning the DA. */
UCLASS(Transient)
class UWeaponDefinitionActorFactory : public UActorFactory
{
	GENERATED_BODY()

public:
	UWeaponDefinitionActorFactory();

	void Configure(UWeaponDefinition* InWeaponDefinition);

	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;

protected:
	virtual FString GetDefaultActorLabel(UObject* Asset) const override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UWeaponDefinition> WeaponDefinition = nullptr;
};
