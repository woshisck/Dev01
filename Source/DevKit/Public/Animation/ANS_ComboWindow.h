#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_ComboWindow.generated.h"

class UAbilitySystemComponent;

/**
 * AnimNotifyState: combo input window for UGameplayAbilityComboGraph.
 *
 * Place this state on a montage to define the frame range in which the player
 * is allowed to trigger the next combo node.
 *
 * NotifyBegin → adds   PlayerState.AbilityCast.CanCombo (opens the window)
 * NotifyEnd   → removes PlayerState.AbilityCast.CanCombo (closes the window)
 *
 * GA_PlayMontage::OnCanComboTagChanged fires on Begin and checks the input
 * buffer to immediately chain if a buffered attack input exists.
 */
UCLASS(meta = (DisplayName = "ANS Combo Window"))
class DEVKIT_API UANS_ComboWindow : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;

private:
    static UAbilitySystemComponent* GetASC(USkeletalMeshComponent* MeshComp);
};
