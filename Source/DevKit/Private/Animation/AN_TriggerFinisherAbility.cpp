#include "Animation/AN_TriggerFinisherAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"

// 检测 WindowOpen Tag（而非 FinisherCharge），确保最后一层命中消耗GE后仍能触发终结技
static const FGameplayTag TAG_AN_TriggerFinisherAbility_WindowOpen =
    FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherWindowOpen"));

static const FGameplayTag TAG_AN_TriggerFinisherAbility_PlayerFinisherAttack =
    FGameplayTag::RequestGameplayTag(TEXT("Action.Player.FinisherAttack"));

void UAN_TriggerFinisherAbility::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        return;
    }

    AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
    if (!Character || !Character->GetASC())
    {
        return;
    }

    // 检测 WindowOpen Tag（GA_FinisherCharge EndAbility 时移除，比GE Tag更晚消失）
    if (!Character->GetASC()->HasMatchingGameplayTag(TAG_AN_TriggerFinisherAbility_WindowOpen))
    {
        return;
    }

    FGameplayEventData Payload;
    Payload.Instigator = Character;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, TAG_AN_TriggerFinisherAbility_PlayerFinisherAttack, Payload);
}

FString UAN_TriggerFinisherAbility::GetNotifyName_Implementation() const
{
    return TEXT("Trigger Finisher Ability");
}
