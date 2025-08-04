// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Item.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UItemDefinition;

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AItem : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItem();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item | InterAct")
	TObjectPtr<UCapsuleComponent> PlayerInteractVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item | ItemDisplay")
	TObjectPtr<UCapsuleComponent> BlockVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item | ItemDisplay")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item | ItemData")
	TObjectPtr<UItemDefinition> ItemDefinition;

};
