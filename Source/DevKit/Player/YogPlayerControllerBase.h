// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>

#include "EnhancedInputComponent.h"
#include "InputAction.h"

#include "YogPlayerControllerBase.generated.h"


class AYogCameraPawn;
class AYogCharacterBase;
class UInputMappingContext;




UCLASS()
class DEVKIT_API AYogPlayerControllerBase : public AModularPlayerController
{
	GENERATED_BODY()

public:
	AYogPlayerControllerBase(){}

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;

	//UFUNCTION(BlueprintCallable)
	//AYogCharacterBase* GetPossCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Setting")
	TSubclassOf<AYogCameraPawn> CameraPawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/////////////////////////////////////////// INPUT ACTION ///////////////////////////////////////////

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> action_LightAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> action_HeavyAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputAction> action_Move;

	/////////////////////////////////////////// INPUT ACTION ///////////////////////////////////////////

	UFUNCTION(BlueprintImplementableEvent)
	void OnMoveTriggered(const FInputActionValue& Value);

	UFUNCTION(BlueprintImplementableEvent)
	void OnHeavyAttackTriggered(const FInputActionValue& Value);

	UFUNCTION(BlueprintImplementableEvent)
	void OnLightAttackTriggered(const FInputActionValue& Value);

	/////////////////////////////////////////// INPUT ACTION ///////////////////////////////////////////

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

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AYogCameraPawn> CameraPawnActor;


};
