// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "AbilitySystem/Attribute/WeaponAttributeSet.h"

#include "PlayerCharacterBase.generated.h"

/**
 * 
 */
class AAuraBase;
class UYogSaveGame;

UENUM()
enum class EPlayerState : uint8
{
	OnMove			UMETA(DisplayName = "OnMove"),
	OnAction		UMETA(DisplayName = "OnAction")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerStateDelegate, EPlayerState, State);


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemInteractEndDelegate, APlayerCharacterBase*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemInteractStartDelegate, APlayerCharacterBase*, Character, AActor*, actor);




UCLASS()
class DEVKIT_API APlayerCharacterBase : public AYogCharacterBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	APlayerCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
	void SetOwnCamera(AYogCameraPawn* cameraActor);

	UFUNCTION(BlueprintCallable)
	AYogCameraPawn* GetOwnCamera();

	UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual void Die() override;

public:

	UPROPERTY()
	TSubclassOf<APlayerCharacterBase> PlayerBlueprintClass;



	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UPlayerAttributeSet> PlayerAttributeSet;

	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FItemInteractStartDelegate OnItemInterActionStart;

	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FItemInteractEndDelegate OnItemInterActionEnd;

	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FPlayerStateDelegate OnPlayerStateUpdate;


	UPROPERTY(BlueprintAssignable, Category = "State")
	FPlayerStateDelegate OnFPlayerStateDeleg;


	UPROPERTY()
	TObjectPtr<AActor> temp_Item_prepare;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura")
	TSubclassOf<AAuraBase> Aura;


	friend UPlayerAttributeSet;


protected:

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AYogCameraPawn> CameraPawnActor;



};
