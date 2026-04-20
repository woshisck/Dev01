#include "AI/BTDecorator_HasAbilityWithTag.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTDecorator_HasAbilityWithTag::UBTDecorator_HasAbilityWithTag()
{
    NodeName = TEXT("Has Ability With Tag");
    bNotifyBecomeRelevant = false;
    bNotifyCeaseRelevant = false;
}

bool UBTDecorator_HasAbilityWithTag::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    const AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC) return false;

    const IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(AIC->GetPawn());
    const UAbilitySystemComponent* ASC = ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
    if (!ASC) return false;

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (Spec.Ability && Spec.Ability->AbilityTags.HasAny(AbilityTags))
        {
            return true;
        }
    }
    return false;
}

FString UBTDecorator_HasAbilityWithTag::GetStaticDescription() const
{
    return FString::Printf(TEXT("Has Ability: %s"), *AbilityTags.ToStringSimple());
}
