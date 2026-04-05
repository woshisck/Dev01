#include "BuffFlow/Nodes/BFNode_GrantGA.h"
#include "AbilitySystemComponent.h"

UBFNode_GrantGA::UBFNode_GrantGA(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_GrantGA::ExecuteInput(const FName& PinName)
{
	if (!AbilityClass)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FGameplayAbilitySpec Spec(AbilityClass, AbilityLevel);
	GrantedHandle = ASC->GiveAbility(Spec);

	if (!GrantedHandle.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	GrantedASC = ASC;
	TriggerOutput(TEXT("Out"), true);
}

void UBFNode_GrantGA::Cleanup()
{
	// FA 停止时自动撤销 GA（符文卸下、BuffFlow 终止、场景切换等均触发）
	if (GrantedHandle.IsValid() && GrantedASC.IsValid())
	{
		GrantedASC->ClearAbility(GrantedHandle);
	}
	GrantedHandle = FGameplayAbilitySpecHandle();
	GrantedASC.Reset();

	Super::Cleanup();
}
