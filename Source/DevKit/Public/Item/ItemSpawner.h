// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Character/PlayerInteraction.h"
#include "ItemDefinition.h"
#include "ItemSpawner.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class APlayerCharacterBase;

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AItemSpawner : public AActor, public IPlayerInteraction
{
	GENERATED_BODY()

public:
	AItemSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;
	void OnConstruction(const FTransform& Transform) override;

	// --- IPlayerInteraction ---
	virtual void OnPlayerBeginOverlap(APlayerCharacterBase* Player) override;
	virtual void OnPlayerEndOverlap(APlayerCharacterBase* Player) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item | ItemData")
	TObjectPtr<UItemDefinition> ItemDefinition;

	UPROPERTY(EditDefaultsOnly, Category = Equipment)
	TObjectPtr<AActor> ActorsToSpawn;

	UFUNCTION(BlueprintNativeEvent)
	void GrantItem(AYogCharacterBase* ReceivingChar);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item | InterAct")
	TObjectPtr<UCapsuleComponent> PlayerInteractVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item | ItemDisplay")
	TObjectPtr<UCapsuleComponent> BlockVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item | ItemDisplay")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
