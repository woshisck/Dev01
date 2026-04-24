#include "AbilitySystem/Abilities/GA_Fear.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"

UGA_Fear::UGA_Fear(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Feared")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Feared")));

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = FGameplayTag::RequestGameplayTag(TEXT("Buff.Event.Fear"));
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Fear::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	OriginLocation    = Avatar->GetActorLocation();
	InstigatorLocation = TriggerEventData && TriggerEventData->Instigator
		? TriggerEventData->Instigator->GetActorLocation()
		: OriginLocation + FVector(100.f, 0.f, 0.f);

	PenaltyDamage = (TriggerEventData && TriggerEventData->EventMagnitude > 0.f)
		? TriggerEventData->EventMagnitude : DefaultPenaltyDamage;

	if (UWorld* World = GetWorld())
	{
		// 每 MoveUpdateInterval 强制移动方向
		World->GetTimerManager().SetTimer(
			MoveTimerHandle, this, &UGA_Fear::UpdateFearMovement,
			MoveUpdateInterval, true, 0.f);

		// FearDuration 后检查距离
		World->GetTimerManager().SetTimer(
			CheckTimerHandle, this, &UGA_Fear::OnFearTimeExpired,
			FearDuration, false);
	}
}

void UGA_Fear::UpdateFearMovement()
{
	if (!CurrentActorInfo || !CurrentActorInfo->AvatarActor.IsValid()) return;

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	ACharacter* Character = Cast<ACharacter>(Avatar);
	if (!Character) return;

	// 逃离方向：远离 Instigator（持续更新 InstigatorLocation 可选，此处固定）
	FVector AwayDir = (Avatar->GetActorLocation() - InstigatorLocation);
	AwayDir.Z = 0.f;
	if (AwayDir.IsNearlyZero()) AwayDir = Character->GetActorForwardVector();
	AwayDir.Normalize();

	// 对 AI 敌人：通过 AIController 发出 MoveToLocation
	if (AAIController* AICon = Cast<AAIController>(Character->GetController()))
	{
		const FVector FleeTarget = Avatar->GetActorLocation() + AwayDir * 1200.f;
		AICon->MoveToLocation(FleeTarget, 50.f, false, true, false, true);
	}
	else
	{
		// 对非AI角色（理论上玩家应被 ActivationBlockedTags 拦截）：AddMovementInput
		Character->AddMovementInput(AwayDir, 1.f, true);
	}
}

void UGA_Fear::OnFearTimeExpired()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MoveTimerHandle);
	}

	// 检查逃离距离
	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	if (Avatar)
	{
		const float DistFromOrigin = FVector::Dist2D(Avatar->GetActorLocation(), OriginLocation);
		if (DistFromOrigin < RequiredDistance && PenaltyDamageEffect)
		{
			UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
			if (ASC)
			{
				FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
				FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(PenaltyDamageEffect, 1.f, Ctx);
				if (Spec.IsValid())
				{
					static const FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
					if (DataTag.IsValid())
					{
						Spec.Data->SetSetByCallerMagnitude(DataTag, PenaltyDamage);
					}
					ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				}
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Fear::OnFearTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Fear::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CheckTimerHandle);
		World->GetTimerManager().ClearTimer(MoveTimerHandle);
	}

	// 停止 AI 的 Fear 移动，让 BT 恢复控制
	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			if (AAIController* AICon = Cast<AAIController>(Character->GetController()))
			{
				AICon->StopMovement();
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
