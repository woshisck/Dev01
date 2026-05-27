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
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")), FFlowPin(TEXT("Removed")), FFlowPin(TEXT("OnRemoved")) };
	Level = FFlowDataPinInputProperty_Float(1.f);
	StacksToRemove = FFlowDataPinInputProperty_Int32(1);
}

void UBFNode_ApplyEffect::Cleanup()
{
	bCleaningUp = true;

	if (bRemoveEffectOnCleanup && GrantedASC.IsValid() && GrantedHandle.IsValid())
	{
		GrantedASC->RemoveActiveGameplayEffect(GrantedHandle);
	}
	GrantedHandle = FActiveGameplayEffectHandle();
	GrantedASC.Reset();

	if (bRemoveEffectOnCleanup)
	{
		for (FMultiTargetEntry& Entry : MultiTargetHandles)
		{
			if (Entry.ASC.IsValid() && Entry.Handle.IsValid())
			{
				Entry.ASC->RemoveActiveGameplayEffect(Entry.Handle);
			}
		}
	}
	MultiTargetHandles.Reset();

	Super::Cleanup();
}

void UBFNode_ApplyEffect::HandleGERemoved(const FGameplayEffectRemovalInfo& /*RemovalInfo*/)
{
	if (!bCleaningUp)
	{
		TriggerOutput(TEXT("OnRemoved"), true);
	}
}

void UBFNode_ApplyEffect::ExecuteBuffFlowInput(const FName& PinName)
{
	// ── Remove 引脚处理 ───────────────────────────────────────────────
	if (PinName == TEXT("Remove"))
	{
		// Single-target path
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

		// Multi-target path (AllHitTargets): remove from all tracked targets
		for (FMultiTargetEntry& Entry : MultiTargetHandles)
		{
			if (Entry.ASC.IsValid() && Entry.Handle.IsValid())
			{
				Entry.ASC->RemoveActiveGameplayEffect(Entry.Handle);
			}
		}
		MultiTargetHandles.Reset();

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

	TArray<AActor*> TargetActors = ResolveAllTargets(Target);
	if (TargetActors.IsEmpty())
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

	const int32 ResolvedApplicationCount = FMath::Clamp(ApplicationCount, 1, 20);
	const bool bMultiTarget = Target == EBFTargetSelector::AllHitTargets;

	FActiveGameplayEffectHandle LastHandle;
	UAbilitySystemComponent* LastTargetASC = nullptr;

	for (AActor* TargetActor : TargetActors)
	{
		// 获取目标的 ASC（支持任何实现了 IAbilitySystemInterface 的 Actor）
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC)
		{
			continue;
		}

		FActiveGameplayEffectHandle Handle;
		for (int32 ApplyIndex = 0; ApplyIndex < ResolvedApplicationCount; ++ApplyIndex)
		{
			Handle = TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}

		if (Handle.IsValid())
		{
			LastHandle    = Handle;
			LastTargetASC = TargetASC;

			if (bMultiTarget)
			{
				// AllHitTargets：为每个目标单独跟踪 handle
				FMultiTargetEntry& Entry = MultiTargetHandles.AddDefaulted_GetRef();
				Entry.ASC    = TargetASC;
				Entry.Handle = Handle;

				FOnActiveGameplayEffectRemoved_Info* Del = TargetASC->OnGameplayEffectRemoved_InfoDelegate(Handle);
				if (Del)
				{
					Del->AddUObject(this, &UBFNode_ApplyEffect::HandleGERemoved);
				}
			}
			else if (!GrantedHandle.IsValid())
			{
				// 单目标：仅存储首次有效 handle
				GrantedHandle = Handle;
				GrantedASC    = TargetASC;

				FOnActiveGameplayEffectRemoved_Info* Del = TargetASC->OnGameplayEffectRemoved_InfoDelegate(Handle);
				if (Del)
				{
					Del->AddUObject(this, &UBFNode_ApplyEffect::HandleGERemoved);
				}
			}
		}
	}

	// 至少有一个目标失败（整体按 Failed 处理仅当所有目标均无 ASC）
	if (!LastHandle.IsValid() && !bMultiTarget)
	{
		bGEApplied      = FFlowDataPinOutputProperty_Bool(false);
		GEStackCount    = FFlowDataPinOutputProperty_Int32(0);
		GELevel         = FFlowDataPinOutputProperty_Float(0.f);
		GETimeRemaining = FFlowDataPinOutputProperty_Float(0.f);
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// ── 写入输出数据引脚（反映最后一次成功施加的状态） ──────────────────
	if (LastHandle.IsValid())
	{
		bGEApplied = FFlowDataPinOutputProperty_Bool(true);

		const FActiveGameplayEffect* ActiveGE = LastTargetASC->GetActiveGameplayEffect(LastHandle);
		if (ActiveGE)
		{
			GEStackCount    = FFlowDataPinOutputProperty_Int32(LastTargetASC->GetCurrentStackCount(LastHandle));
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
		// AllHitTargets 且所有目标均已施加（部分 Instant GE handle 失效属正常）
		bGEApplied      = FFlowDataPinOutputProperty_Bool(!MultiTargetHandles.IsEmpty());
		GEStackCount    = FFlowDataPinOutputProperty_Int32(0);
		GELevel         = FFlowDataPinOutputProperty_Float(ResolvedLevel);
		GETimeRemaining = FFlowDataPinOutputProperty_Float(0.f);
	}

	// bFinish=false：保持节点活跃，确保 FA 停止时 Cleanup() 被调用
	TriggerOutput(TEXT("Out"), false);
}
