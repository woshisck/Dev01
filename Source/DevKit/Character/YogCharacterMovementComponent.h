// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"

#include "YogCharacterMovementComponent.generated.h"


class UObject;
struct FFrame;

//DEVKIT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_MovementStopped);
/**
 * 
 */
UCLASS(Config = Game)
class DEVKIT_API UYogCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UYogCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);


//~UMovementComponent interface
	virtual FRotator GetDeltaRotation(float DeltaTime) const override;
	virtual float GetMaxSpeed() const override;
//~End of UMovementComponent interface


protected:
	virtual void InitializeComponent() override;

protected:

};
