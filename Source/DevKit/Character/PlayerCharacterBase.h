// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "YogCharacterBase.h"
#include "PlayerCharacterBase.generated.h"

/**
 * 
 */


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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DT")
	TObjectPtr<UDataTable> CharacterMovementDataTable;


};
