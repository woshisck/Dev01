// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "Engine/World.h"


#include "YogEquipmentInstance.generated.h"



class AActor;
class APawn;
struct FYogEquipmentActorToSpawn;
/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UYogEquipmentInstance : public UObject
{
	GENERATED_BODY()
public:
	UYogEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

//~UObject interface
	//virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override final;
//~End of UObject interface

	UFUNCTION(BlueprintPure, Category = Equipment)
	UObject* GetInstigator() const { return Instigator; }

	void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }

	UFUNCTION(BlueprintPure, Category = Equipment)
	APawn* GetPawn() const;

	UFUNCTION(BlueprintPure, Category = Equipment, meta = (DeterminesOutputType = PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;

	virtual void SpawnEquipmentActors(const TArray<FYogEquipmentActorToSpawn>& ActorsToSpawn);
	virtual void DestroyEquipmentActors();

	virtual void OnEquipped(FTransform& SpawnLoc);
	virtual void OnUnequipped();


protected:
	UFUNCTION(BlueprintImplementableEvent, Category = Equipment, meta = (DisplayName = "OnEquipped"))
	void K2_OnEquipped(const FTransform& SpawnLoc);

	UFUNCTION(BlueprintImplementableEvent, Category = Equipment, meta = (DisplayName = "OnUnequipped"))
	void K2_OnUnequipped();



private:
	UPROPERTY()
	TObjectPtr<UObject> Instigator;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;
};
