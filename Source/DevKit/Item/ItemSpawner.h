// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemDefinition.h"


#include "ItemSpawner.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;


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

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item | ItemData")
	TObjectPtr<UItemDefinition> ItemDefinition;

	UPROPERTY(EditDefaultsOnly, Category = Equipment)
	TObjectPtr<AActor> ActorsToSpawn;


	UFUNCTION(BlueprintNativeEvent)
	void GrantItem(AYogCharacterBase* ReceivingChar);


	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	//OnComponentEndOverlap.Broadcast(this, OtherActor, OtherComp, OtherOverlap.GetBodyIndex());


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item | InterAct")
	TObjectPtr<UCapsuleComponent> PlayerInteractVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item | ItemDisplay")
	TObjectPtr<UCapsuleComponent> BlockVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item | ItemDisplay")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

};
