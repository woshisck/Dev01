#include "Component/SkillChargeComponent.h"
#include "AbilitySystemComponent.h"

USkillChargeComponent::USkillChargeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USkillChargeComponent::InitWithASC(UAbilitySystemComponent* InASC)
{
	ASC = InASC;
}

void USkillChargeComponent::RegisterSkill(FGameplayTag SkillTag, FGameplayAttribute MaxChargeAttr, FGameplayAttribute CDDurationAttr)
{
	if (!ASC.IsValid()) return;

	FSkillChargeState& State = ChargeStates.Add(SkillTag);
	State.MaxChargeAttr    = MaxChargeAttr;
	State.CDDurationAttr   = CDDurationAttr;
	State.ChargesInRecovery = 0;

	// 初始充能 = 上限（满格开始）
	State.CurrentCharge = GetMaxChargeValue(State);

	// 监听 MaxCharge 变化：符文离区时 GE 撤销导致 Max 下降，需要钳制 Current
	ASC->GetGameplayAttributeValueChangeDelegate(MaxChargeAttr)
		.AddLambda([this, SkillTag](const FOnAttributeChangeData& Data)
		{
			FSkillChargeState* StatePtr = ChargeStates.Find(SkillTag);
			if (!StatePtr) return;

			const int32 NewMax = FMath::Max(1, FMath::RoundToInt(Data.NewValue));
			if (StatePtr->CurrentCharge > NewMax)
			{
				StatePtr->CurrentCharge = NewMax;
				OnChargeChanged.Broadcast(SkillTag, StatePtr->CurrentCharge);
			}
		});
}

bool USkillChargeComponent::HasCharge(FGameplayTag SkillTag) const
{
	const FSkillChargeState* State = ChargeStates.Find(SkillTag);
	return State && State->CurrentCharge > 0;
}

bool USkillChargeComponent::ConsumeCharge(FGameplayTag SkillTag)
{
	FSkillChargeState* State = ChargeStates.Find(SkillTag);
	if (!State || State->CurrentCharge <= 0) return false;

	State->CurrentCharge--;
	State->ChargesInRecovery++;
	OnChargeChanged.Broadcast(SkillTag, State->CurrentCharge);

	// 队列模式：只有从 0→1 时才需要启动 Timer；
	// 已有 Timer 在跑时，它结束后会自动检查 ChargesInRecovery 并继续。
	if (State->ChargesInRecovery == 1)
	{
		const float CDDuration = GetCDDurationValue(*State);
		FTimerDelegate Delegate = FTimerDelegate::CreateUObject(
			this, &USkillChargeComponent::RecoveryTick, SkillTag);
		GetWorld()->GetTimerManager().SetTimer(State->RecoveryTimer, Delegate, CDDuration, false);
	}

	return true;
}

int32 USkillChargeComponent::GetCurrentCharge(FGameplayTag SkillTag) const
{
	const FSkillChargeState* State = ChargeStates.Find(SkillTag);
	return State ? State->CurrentCharge : 0;
}

int32 USkillChargeComponent::GetMaxCharge(FGameplayTag SkillTag) const
{
	const FSkillChargeState* State = ChargeStates.Find(SkillTag);
	return State ? GetMaxChargeValue(*State) : 0;
}

int32 USkillChargeComponent::GetMaxChargeValue(const FSkillChargeState& State) const
{
	if (!ASC.IsValid()) return 1;
	bool bFound = false;
	const float Value = ASC->GetGameplayAttributeValue(State.MaxChargeAttr, bFound);
	return bFound ? FMath::Max(1, FMath::RoundToInt(Value)) : 1;
}

float USkillChargeComponent::GetCDDurationValue(const FSkillChargeState& State) const
{
	if (!ASC.IsValid()) return 3.0f;
	bool bFound = false;
	const float Value = ASC->GetGameplayAttributeValue(State.CDDurationAttr, bFound);
	return (bFound && Value > 0.f) ? Value : 3.0f;
}

void USkillChargeComponent::RecoveryTick(FGameplayTag SkillTag)
{
	FSkillChargeState* State = ChargeStates.Find(SkillTag);
	if (!State) return;

	// 回复一格
	const int32 Max = GetMaxChargeValue(*State);
	State->CurrentCharge = FMath::Min(State->CurrentCharge + 1, Max);
	State->ChargesInRecovery--;
	OnChargeChanged.Broadcast(SkillTag, State->CurrentCharge);

	// 队列中还有待回复的格，继续启动下一个 Timer
	// 每次重新读取 CDDuration，支持符文动态改 CDR
	if (State->ChargesInRecovery > 0)
	{
		const float CDDuration = GetCDDurationValue(*State);
		FTimerDelegate Delegate = FTimerDelegate::CreateUObject(
			this, &USkillChargeComponent::RecoveryTick, SkillTag);
		GetWorld()->GetTimerManager().SetTimer(State->RecoveryTimer, Delegate, CDDuration, false);
	}
}