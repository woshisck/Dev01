#pragma once

#include "ActorFactories/ActorFactory.h"

#include "EnemyDataActorFactory.generated.h"

class UEnemyData;

/** Places an EnemyData asset by spawning its configured EnemyClass and assigning the DA. */
UCLASS(Transient)
class UEnemyDataActorFactory : public UActorFactory
{
	GENERATED_BODY()

public:
	UEnemyDataActorFactory();

	void Configure(UEnemyData* InEnemyData);

	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual UClass* GetDefaultActorClass(const FAssetData& AssetData) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;

protected:
	virtual FString GetDefaultActorLabel(UObject* Asset) const override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UEnemyData> EnemyData = nullptr;
};
