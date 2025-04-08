// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>


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

	UFUNCTION(BlueprintCallable)
	AYogCharacterBase* GetPossCharacter();



	UFUNCTION(BlueprintCallable, Category = "ASC")
	UYogAbilitySystemComponent* GetYogAbilitySystemComponent() const;


	UFUNCTION(BlueprintCallable, Category = "ASC")
	void SetEnableRotationRate(FRotator RotationRate, bool isEnable);

	UFUNCTION(BlueprintCallable, Category = "ASC")
	void SpawnCameraPawn(AYogCharacterBase* PossessedCharacter);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Setting")
	TSubclassOf<AYogCameraPawn> CameraPawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetPlayerState(EYogCharacterState newState);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Setting")
	TObjectPtr<AYogCameraPawn> CameraPawnActor;
};
