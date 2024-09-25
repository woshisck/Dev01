// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GSCTriggerComboNotify.generated.h"

/**
 * Use this notify to tell the system to trigger the next combo activation if the player registered an input within the combo window notify state.
 *
 *
 * Note that this notify must be placed within the combo window to properly work.
 */
UCLASS()
class GASCOMPANION_API UGSCTriggerComboNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	// Used to check if the owner actor of this notify is the preview actor of Persona, in which case we don't do anything
	// to prevent log warning when getting components via Companion interfaces
	FString AnimationEditorPreviewActorString = "AnimationEditorPreviewActor";

	AActor* GetOwnerActor(USkeletalMeshComponent* MeshComponent) const;
};
