// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Components/GSCComboManagerComponent.h"

#include "GSCComboWindowNotifyState.generated.h"

/**
 * Use this notify state to open a combo window during witch the player can queue up the next combo by activating the ability again.
 *
 * Don't forget to set the `bEndCombo` property to true on this notifier if the montage is the last one of your combo chain.
 */
UCLASS()
class GASCOMPANION_API UGSCComboWindowNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** Whether this montage is ending a combo (last montage in the combo chain) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	bool bEndCombo = false;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;

	virtual FString GetEditorComment() override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	// Used to check if the owner actor of this notify is the preview actor of Persona, in which case we don't do anything
	// to prevent log warning when getting components via Companion interfaces
	FString AnimationEditorPreviewActorString = "AnimationEditorPreviewActor";

	AActor* GetOwnerActor(USkeletalMeshComponent* MeshComponent) const;
};
