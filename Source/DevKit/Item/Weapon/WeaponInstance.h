// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "WeaponInstance.generated.h"

/**
 * 
 */
struct FWeaponActorToSpawn;


UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UWeaponInstance : public UObject
{
	GENERATED_BODY()
public:
	UWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	virtual UWorld* GetWorld() const override final;
	//~End of UObject interface

	UFUNCTION(BlueprintPure, Category = Equipment)
	UObject* GetInstigator() const { return Instigator; }

	void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }

	UFUNCTION(BlueprintPure, Category = Equipment)
	APawn* GetPawn() const;

	UFUNCTION(BlueprintPure, Category = Equipment, meta = (DeterminesOutputType = PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;

	UFUNCTION(BlueprintPure, Category = Equipment)
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	void SpawnEquipmentActors(const TArray<FWeaponActorToSpawn>& ActorsToSpawn);
	void DestroyEquipmentActors();

	virtual void OnEquipped();
	virtual void OnUnequipped();


	UPROPERTY(BlueprintReadWrite, meta = (DeterminesOutputType = PawnType))
	TObjectPtr<UObject> Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Equipment)
	TArray<TObjectPtr<AActor>> SpawnedActors;

	UFUNCTION(BlueprintImplementableEvent, Category = Equipment, meta = (DisplayName = "OnEquipped"))
	void K2_OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, Category = Equipment, meta = (DisplayName = "OnUnequipped"))
	void K2_OnUnequipped();

};
