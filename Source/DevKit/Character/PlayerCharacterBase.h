// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "YogCharacterBase.h"
#include "PlayerCharacterBase.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemInteractStartDelegate, APlayerCharacterBase*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemInteractEndDelegate, APlayerCharacterBase*, Character);

UENUM()
enum class EPlayerBattleState : uint8
{
	OnGetHit		UMETA(DisplayName = "OnGetHit"),
	OnHitFrame		UMETA(DisplayName = "OnHitFrame")
};

UCLASS()
class DEVKIT_API APlayerCharacterBase : public AYogCharacterBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;


	APlayerCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FItemInteractStartDelegate OnItemInterActionStart;

	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FItemInteractEndDelegate OnItemInterActionEnd;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DT")
	TObjectPtr<UDataTable> CharacterMovementDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle Data")
	float EnemyCloseDist;


	UFUNCTION(BlueprintCallable)
	void SetOwnCamera(AYogCameraPawn* cameraActor);

	UFUNCTION(BlueprintCallable)
	AYogCameraPawn* GetOwnCamera();

protected:

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AYogCameraPawn> CameraPawnActor;
};
