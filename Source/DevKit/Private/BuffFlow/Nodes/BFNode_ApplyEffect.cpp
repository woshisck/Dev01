#include "BuffFlow/Nodes/BFNode_ApplyEffect.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Types/FlowDataPinResults.h"

UBFNode_ApplyEffect::UBFNode_ApplyEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Remove")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")), FFlowPin(TEXT("Removed")) };
	Level = FFlowDataPinInputProperty_Float(1.f);
	StacksToRemove = FFlowDataPinInputProperty_Int32(1);
}

void UBFNode_ApplyEffect::Cleanup()
{
	if (GrantedASC.IsValid() && GrantedHandle.IsValid())
	{
		GrantedASC->RemoveActiveGameplayEffect(GrantedHandle);
	}
	GrantedHandle = FActiveGameplayEffectHandle();
	GrantedASC.Reset();
	Super::Cleanup();
}

void UBFNode_ApplyEffect::ExecuteInput(const FName& PinName)
{
	// ── Remove 引脚处理 ───────────────────────────────────────────────
	if (PinName == TEXT("Remove"))
	{
		if (GrantedASC.IsValid() && GrantedHandle.IsValid())
		{
			int32 Count = -1; // -1 = 全部移除
			if (RemoveMode == EBFRemoveMode::OneStack)
			{
				Count = 1;
			}
			else if (RemoveMode == EBFRemoveMode::CustomCount)
			{
				FFlowDataPinResult_Int Res = TryResolveDataPinAsInt(GET_MEMBER_NAME_CHECKED(UBFNode_ApplyEffect, StacksToRemove));
				Count = (Res.Result == EFlowDataPinResolveResult::Success) ? static_cast<int32>(Res.Value) : StacksToRemove.Value;
				Count = FMath::Max(Count, 1);
			}

			GrantedASC->RemoveActiveGameplayEffect(GrantedHandle, Count);

			// AllStacks 或移除后层数归零时重置 handle
			if (Count == -1 || GrantedASC->GetCurrentStackCount(GrantedHandle) <= 0)
			{
				GrantedHandle = FActiveGameplayEffectHandle();
				GrantedASC.Reset();
			}
		}
		TriggerOutput(TEXT("Removed"), true);
		return;
	}

	// ── In 引脚处理（施加 GE）────────────────────────────────────────
	if (!Effect)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UYogAbilitySystemComponent* OwnerASC = GetOwnerASC();
	if (!OwnerASC)
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

	// 获取目标的 ASC（支持任何实现了 IAbilitySystemInterface 的 Actor）
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 优先从连接的数据引脚读取 Level，无连接则使用节点上填写的值
	FFlowDataPinResult_Float LevelResult = TryResolveDataPinAsFloat(GET_MEMBER_NAME_CHECKED(UBFNode_ApplyEffect, Level));
	const float ResolvedLevel = (LevelResult.Result == EFlowDataPinResolveResult::Success) ? LevelResult.Value : Level.Value;

	// 以 BuffOwner 为来源构建 Spec，施加到目标
	FGameplayEffectContextHandle Context = OwnerASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = OwnerASC->MakeOutgoingSpec(Effect, ResolvedLevel, Context);
	if (!Spec.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 填写 SetByCaller 槽位（Tag 非空才赋值）
	auto ApplySetByCaller = [&](const FGameplayTag& Tag, const FName& PinMemberName, const FFlowDataPinInputProperty_Float& DefaultVal)
	{
		if (!Tag.IsValid()) return;
		FFlowDataPinResult_Float Res = TryResolveDataPinAsFloat(PinMemberName);
		const float Val = (Res.Result == EFlowDataPinResolveResult::Success) ? Res.Value : DefaultVal.Value;
		Spec.Data->SetSetByCallerMagnitude(Tag, Val);
	};
	ApplySetByCaller(SetByCallerTag1, GET_MEMBER_NAME_CHECKED(UBFNode_ApplyEffect, SetByCallerValue1), SetByCallerValue1);
	ApplySetByCaller(SetByCallerTag2, GET_MEMBER_NAME_CHECKED(UBFNode_ApplyEffect, SetByCallerValue2), SetByCallerValue2);
	ApplySetByCaller(SetByCallerTag3, GET_MEMBER_NAME_CHECKED(UBFNode_ApplyEffect, SetByCallerValue3), SetByCallerValue3);

	FActiveGameplayEffectHandle Handle = TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	// 仅存储首次有效 handle（Unique/Stackable GE 始终是同一实例，Instant GE handle 无效）
	if (!GrantedHandle.IsValid() && Handle.IsValid())
	{
		GrantedHandle = Handle;
		GrantedASC    = TargetASC;
	}

	// ── 写入输出数据引脚（反映施加瞬间状态） ──────────────────────────
	if (Handle.IsValid())
	{
		bGEApplied = FFlowDataPinOutputProperty_Bool(true);

		const FActiveGameplayEffect* ActiveGE = TargetASC->GetActiveGameplayEffect(Handle);
		if (ActiveGE)
		{
			GEStackCount    = FFlowDataPinOutputProperty_Int32(TargetASC->GetCurrentStackCount(Handle));
			GELevel         = FFlowDataPinOutputProperty_Float(ActiveGE->Spec.GetLevel());
			const float Dur = ActiveGE->GetDuration();
			GETimeRemaining = FFlowDataPinOutputProperty_Float(Dur < 0.f ? -1.f : Dur);
		}
		else
		{
			// Instant GE：handle 施加后立即失效，视为单次成功
			GEStackCount    = FFlowDataPinOutputProperty_Int32(1);
			GELevel         = FFlowDataPinOutputProperty_Float(ResolvedLevel);
			GETimeRemaining = FFlowDataPinOutputProperty_Float(0.f);
		}
	}
	else
	{
		bGEApplied      = FFlowDataPinOutputProperty_Bool(false);
		GEStackCount    = FFlowDataPinOutputProperty_Int32(0);
		GELevel         = FFlowDataPinOutputProperty_Float(0.f);
		GETimeRemaining = FFlowDataPinOutputProperty_Float(0.f);
	}

	// bFinish=false：保持节点活跃，确保 FA 停止时 Cleanup() 被调用
	TriggerOutput(TEXT("Out"), false);
}
