#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_SwitchWeapon.generated.h"

/**
 * Commits the player's pending weapon switch on the authored montage frame.
 * GA_SwitchWeapon still performs a one-shot fallback on montage end/interruption.
 */
UCLASS(meta = (DisplayName = "AN Switch Weapon"))
class DEVKIT_API UAN_SwitchWeapon : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
