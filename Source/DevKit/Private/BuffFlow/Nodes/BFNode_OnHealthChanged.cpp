#include "BuffFlow/Nodes/BFNode_OnHealthChanged.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

UBFNode_OnHealthChanged::UBFNode_OnHealthChanged(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Trigger");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("OnHealthChanged")) };
}

void UBFNode_OnHealthChanged::ExecuteBuffFlowInput(const FName& PinName)
{
	if (PinName == TEXT("In"))
	{
		UYogAbilitySystemComponent* ASC = GetOwnerASC();
		if (!ASC) return;

		// 防止重复绑定
		if (BoundASC.IsValid() && HealthChangedHandle.IsValid())
		{
			BoundASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute())
				.Remove(HealthChangedHandle);
		}

		BoundASC = ASC;
		HealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute())
			.AddUObject(this, &UBFNode_OnHealthChanged::HandleHealthChanged);
	}
	else if (PinName == TEXT("Stop"))
	{
		if (BoundASC.IsValid() && HealthChangedHandle.IsValid())
		{
			BoundASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute())
				.Remove(HealthChangedHandle);
			HealthChangedHandle.Reset();
			BoundASC.Reset();
		}
	}
}

void UBFNode_OnHealthChanged::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	NewHealthOutput.Value = Data.NewValue;
	// Trigger output but do NOT finish - keep listening
	TriggerOutput(TEXT("OnHealthChanged"), false);
}

void UBFNode_OnHealthChanged::Cleanup()
{
	if (BoundASC.IsValid() && HealthChangedHandle.IsValid())
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute())
			.Remove(HealthChangedHandle);
		HealthChangedHandle.Reset();
		BoundASC.Reset();
	}

	Super::Cleanup();
}
