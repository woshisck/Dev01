// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_PivotEnd.generated.h"

/**
 * Deprecated, does nothing. The pivot now ends when CharacterMovement velocity reaches zero or flips
 * (see YogLocomotion::NextMotionState). Kept only so the pivot clips that still carry this notify load
 * cleanly; remove it from those clips, then delete this class.
 */
UCLASS(meta = (DisplayName = "AN Pivot End (Deprecated)"))
class DEVKIT_API UAN_PivotEnd : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
};
