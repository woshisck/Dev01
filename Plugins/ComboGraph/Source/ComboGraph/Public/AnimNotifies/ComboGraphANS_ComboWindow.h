// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Runtime/Launch/Resources/Version.h"
#include "ComboGraphANS_ComboWindow.generated.h"

class USkeletalMeshComponent;
struct FGameplayTag;

/** Anim notifies that sends relevant Gameplay Events on begin / end */
UCLASS(NotBlueprintable, meta = (DisplayName = "Combo Graph: Combo Window"))
class COMBOGRAPH_API UComboGraphANS_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
#if ENGINE_MAJOR_VERSION == 5
	
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#else

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

#endif

protected:
	virtual void SendEvent(USkeletalMeshComponent* InMeshComp, FGameplayTag InGameplayTag);
};
