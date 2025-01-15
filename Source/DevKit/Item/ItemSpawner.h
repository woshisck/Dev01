// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemDefinition.h"


#include "ItemSpawner.generated.h"



UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AItemSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItemSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void OnConstruction(const FTransform& Transform) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item | ItemData")
	UItemDefinition* ItemDefinition;

public:
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	UPROPERTY(BlueprintReadOnly, Category = "Item | ItemDisplay")
	TObjectPtr<UCapsuleComponent> CollisionVolume;

	UPROPERTY(BlueprintReadOnly, Category = "Item | ItemDisplay")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

};
