#include "BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h"
#include "AbilitySystemComponent.h"
#include "Types/FlowDataPinResults.h"

UBFNode_ApplyAttributeModifier::UBFNode_ApplyAttributeModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_ApplyAttributeModifier::ExecuteInput(const FName& PinName)
{
	if (!Attribute.IsValid())
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

	// Value：优先读取连入的数据引脚，无连线则使用节点上的固定值
	float ResolvedValue = Value.Value;
	FFlowDataPinResult_Float PinResult = TryResolveDataPinAsFloat(
		GET_MEMBER_NAME_CHECKED(UBFNode_ApplyAttributeModifier, Value));
	if (PinResult.Result == EFlowDataPinResolveResult::Success)
	{
		ResolvedValue = PinResult.Value;
	}

	// 构建 TransientGE（单条属性修改，无需 DA 或 GE 资产）
	UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);

	switch (DurationType)
	{
	case ERuneDurationType::Instant:
		GE->DurationPolicy = EGameplayEffectDurationType::Instant;
		break;
	case ERuneDurationType::Infinite:
		GE->DurationPolicy = EGameplayEffectDurationType::Infinite;
		break;
	case ERuneDurationType::Duration:
		GE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
		GE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));
		break;
	}

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute       = Attribute;
	ModInfo.ModifierOp      = ModOp;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ResolvedValue));
	GE->Modifiers.Add(ModInfo);

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpec Spec(GE, Context, 1.f);
	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(Spec);

	// Instant GE 应用后 handle 即失效，属正常现象；非瞬发 GE handle 无效视为失败
	if (DurationType != ERuneDurationType::Instant && !Handle.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 存储 handle 供 Cleanup 使用（Instant 无需存储）
	if (DurationType != ERuneDurationType::Instant)
	{
		GrantedHandle = Handle;
		GrantedASC    = ASC;
	}

	TriggerOutput(TEXT("Out"), true);
}

void UBFNode_ApplyAttributeModifier::Cleanup()
{
	if (GrantedASC.IsValid() && GrantedHandle.IsValid())
	{
		GrantedASC->RemoveActiveGameplayEffect(GrantedHandle);
	}

	GrantedHandle = FActiveGameplayEffectHandle();
	GrantedASC.Reset();

	Super::Cleanup();
}
