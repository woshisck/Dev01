#include "BuffFlow/Nodes/BFNode_GetRuneInfo.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UBFNode_GetRuneInfo::UBFNode_GetRuneInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Condition");
#endif
	OutputPins = { FFlowPin(TEXT("Found")), FFlowPin(TEXT("NotFound")) };
}

void UBFNode_GetRuneInfo::ExecuteInput(const FName& PinName)
{
	// 默认：未找到
	bIsActive     = FFlowDataPinOutputProperty_Bool(false);
	StackCount    = FFlowDataPinOutputProperty_Int32(0);
	Level         = FFlowDataPinOutputProperty_Float(1.f);
	TimeRemaining = FFlowDataPinOutputProperty_Float(-1.f);

	if (!RuneTag.IsValid())
	{
		TriggerOutput(TEXT("NotFound"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor)
	{
		TriggerOutput(TEXT("NotFound"), true);
		return;
	}

	UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		TriggerOutput(TEXT("NotFound"), true);
		return;
	}

	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(RuneTag);

	TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffectsWithAllTags(TagContainer);
	if (Handles.Num() == 0)
	{
		TriggerOutput(TEXT("NotFound"), true);
		return;
	}

	const FActiveGameplayEffectHandle& Handle = Handles[0];
	const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handle);
	if (!ActiveGE)
	{
		TriggerOutput(TEXT("NotFound"), true);
		return;
	}

	bIsActive  = FFlowDataPinOutputProperty_Bool(true);
	StackCount = FFlowDataPinOutputProperty_Int32(ASC->GetCurrentStackCount(Handle));
	Level      = FFlowDataPinOutputProperty_Float(ActiveGE->Spec.GetLevel());

	const float Duration = ActiveGE->GetDuration();
	if (Duration < 0.f)
	{
		TimeRemaining = FFlowDataPinOutputProperty_Float(-1.f);
	}
	else
	{
		const float WorldTime = TargetActor->GetWorld()->GetTimeSeconds();
		const float Remaining = FMath::Max(0.f, ActiveGE->StartWorldTime + Duration - WorldTime);
		TimeRemaining = FFlowDataPinOutputProperty_Float(Remaining);
	}

	TriggerOutput(TEXT("Found"), true);
}
