// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YogMapDefinition.h"
//#include "DevKit/Player/PlayerCharacterBase.h"

#include "Portal.generated.h"

class APlayerCharacterBase;
class UBillboardComponent;

UCLASS()
class DEVKIT_API APortal : public AActor
{
	GENERATED_BODY()
	
public:	

	APortal(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintImplementableEvent)
	void EnablePortal();

	UFUNCTION(BlueprintImplementableEvent)
	void DisablePortal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	UFUNCTION(BlueprintNativeEvent)
	void EnterPortal(APlayerCharacterBase* ReceivingChar, UYogSaveSubsystem* save_subsystem);


	UFUNCTION(BlueprintCallable)
	void YogOpenLevel(FName level_name);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Index;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ItemPickup")
	TObjectPtr<class UBoxComponent> CollisionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<class UBillboardComponent> BillBoard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GateMap")
	TArray<FNextMapNode> NextLevels;


	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);


};
