// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Runtime/Launch/Resources/Version.h"
#include "ComboGraphANS_CollisionWindow.generated.h"

class USkeletalMeshComponent;
class UComboGraphCollisionComponent;

/** Anim notifies that start and end traces on Owner's ComboGraph CollisionComponent. */
UCLASS(NotBlueprintable, meta = (DisplayName = "Combo Graph: Collision Window"))
class COMBOGRAPH_API UComboGraphANS_CollisionWindow : public UAnimNotifyState
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

	/** Returns owner collision component through component search, if it has any */
	static UComboGraphCollisionComponent* GetCollisionComponent(const AActor* OwnerActor);

	void ActivateCollision(const USkeletalMeshComponent* MeshComp, bool bShouldStart);
};
