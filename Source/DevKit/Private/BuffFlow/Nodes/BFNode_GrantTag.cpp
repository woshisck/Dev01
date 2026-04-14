#include "BuffFlow/Nodes/BFNode_GrantTag.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

UBFNode_GrantTag::UBFNode_GrantTag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Tag");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Remove")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Expired")),
	               FFlowPin(TEXT("Removed")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_GrantTag::ExecuteInput(const FName& PinName)
{
	// ── Remove 引脚：手动提前移除 ────────────────────────────────────────
	if (PinName == TEXT("Remove"))
	{
		if (ExpiryTimer.IsValid())
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(ExpiryTimer);
			}
		}
		RemoveTagFromTarget();
		TriggerOutput(TEXT("Removed"), true);
		return;
	}

	// ── In 引脚：授予 Tag ─────────────────────────────────────────────────
	if (!Tag.IsValid())
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

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	ASC->AddLooseGameplayTag(Tag);
	TotalCountGranted++;
	StoredASC = ASC;

	UE_LOG(LogTemp, Warning, TEXT("[BFNode_GrantTag] Added Tag=%s → %s (Duration=%.1fs)"),
		*Tag.ToString(), *GetNameSafe(TargetActor), Duration);

	// Duration > 0：启动自动过期计时器
	if (Duration > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			// 多次触发 In 引脚时重置计时器（刷新持续时间）
			World->GetTimerManager().ClearTimer(ExpiryTimer);
			World->GetTimerManager().SetTimer(ExpiryTimer, [this]()
			{
				RemoveTagFromTarget();
				TriggerOutput(TEXT("Expired"), true);
			}, Duration, false);
		}
	}

	// bFinish=false：保持活跃，Cleanup 才能处理尚未过期的 Tag
	TriggerOutput(TEXT("Out"), false);
}

void UBFNode_GrantTag::Cleanup()
{
	if (ExpiryTimer.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ExpiryTimer);
		}
	}

	RemoveTagFromTarget();
	Super::Cleanup();
}

void UBFNode_GrantTag::RemoveTagFromTarget()
{
	if (StoredASC.IsValid() && Tag.IsValid() && TotalCountGranted > 0)
	{
		StoredASC->RemoveLooseGameplayTag(Tag, TotalCountGranted);
	}
	TotalCountGranted = 0;
	StoredASC.Reset();
}
