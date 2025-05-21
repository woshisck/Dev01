// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"

#include "YogCharacterMovementComponent.generated.h"


class UObject;
struct FFrame;


USTRUCT(BlueprintType)
struct FYogGroundInfo
{
	GENERATED_BODY()

	FYogGroundInfo()
		: LastUpdateFrame(0)
		, GroundDistance(0.0f)
	{
	}
	uint64 LastUpdateFrame;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance;

};


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

	UFUNCTION(BlueprintCallable, Category = "Lyra|CharacterMovement")
	const FYogGroundInfo& GetGroundInfo();



protected:
	virtual void InitializeComponent() override;

protected:
	// Cached ground info for the character.  Do not access this directly!  It's only updated when accessed via GetGroundInfo().
	FYogGroundInfo CachedGroundInfo;

};
