// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"

#include "YogPlayerControllerBase.generated.h"


class AYogCameraPawn;
class AYogCharacterBase;
class UInputMappingContext;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlayerHasInput, const FVector, Velocity);

UCLASS()
class DEVKIT_API AYogPlayerControllerBase : public AModularPlayerController
{
	GENERATED_BODY()

public:
	AYogPlayerControllerBase(){}

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;


	//UFUNCTION(BlueprintCallable)
	//AYogCharacterBase* GetPossCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Setting")
	TSubclassOf<AYogCameraPawn> CameraPawnClass;


	void LightAtack(const FInputActionValue& Value);
	void HeavyAtack(const FInputActionValue& Value);
	void Dash(const FInputActionValue& Value);
	void Move(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_LightAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_HeavyAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_Dash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> Input_Interact;


	UFUNCTION(BlueprintCallable)
	void OnInteractTriggered(const AItemSpawner* item);

	/////////////////////////////////////////// INPUT ACTION ///////////////////////////////////////////

	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "Character|Movement")
	FPlayerHasInput OnPlayerHasInput;



	UFUNCTION(BlueprintCallable)
	void ToggleInput(bool bEnable);


	UFUNCTION(BlueprintCallable, Category = "ASC")
	UYogAbilitySystemComponent* GetYogAbilitySystemComponent() const;


	UFUNCTION(BlueprintCallable, Category = "ASC")
	void SetEnableRotationRate(FRotator RotationRate, bool isEnable);

	UFUNCTION(BlueprintCallable, Category = "ASC")
	void SpawnCameraPawn(AYogCharacterBase* PossessedCharacter);

	UFUNCTION(BlueprintCallable, Category = "ASC")
	AYogCharacterBase* GetControlledCharacter();

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetPlayerState(EYogCharacterState newState);

private:
	// Enhanced input handle to move forward
	uint32 MoveInputHandle = INDEX_NONE;

	// Enhanced input handle to move forward
	uint32 LightAttackInputHandle = INDEX_NONE;

	// Enhanced input handle to move forward
	uint32 HeavyAttackInputHandle = INDEX_NONE;

	// Enhanced input handle to move forward
	uint32 DashInputHandle = INDEX_NONE;

	// Enhanced input handle to move forward
	uint32 InteractInputHandle = INDEX_NONE;
};
