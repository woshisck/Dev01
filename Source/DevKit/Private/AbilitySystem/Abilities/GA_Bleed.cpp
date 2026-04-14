#include "AbilitySystem/Abilities/GA_Bleed.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameplayTagsManager.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"

UGA_Bleed::UGA_Bleed(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 每个角色只保留一个实例
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// GameplayEvent 触发：FA 通过 Send Gameplay Event（Tag=Buff.Event.Bleed）激活此 GA
	// EventMagnitude 携带每秒伤害量，在 ActivateAbility 里读取
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = FGameplayTag::RequestGameplayTag(TEXT("Buff.Event.Bleed"));
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Bleed::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 监听 Buff.Status.Bleeding 消失 → 自动结束 GA
	// Tag 来源：FA_Rune_Bleed 的 GrantTag(Timed) 到期，或 StopBuffFlow 清理
	const FGameplayTag BleedingTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Bleeding"));
	TagChangeDelegateHandle = ASC->RegisterGameplayTagEvent(
		BleedingTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UGA_Bleed::OnBleedingTagChanged);

	// 从 TriggerEventData 读取每秒伤害，FA 的 Send Gameplay Event.Magnitude 写入此值
	DamagePerSecond = (TriggerEventData && TriggerEventData->EventMagnitude > 0.f)
		? TriggerEventData->EventMagnitude
		: DefaultDamagePerSecond;

	// 缓存发起人 ASC（用于流血伤害日志）
	// BFNode_SendGameplayEvent 将 EventData.Instigator 设为 BuffOwner（玩家）
	InstigatorASC.Reset();
	if (TriggerEventData && TriggerEventData->Instigator != nullptr)
	{
		const AActor* InstigatorActor = TriggerEventData->Instigator.Get();
		if (InstigatorActor)
		{
			if (UYogAbilitySystemComponent* YASC = Cast<UYogAbilitySystemComponent>(
				InstigatorActor->FindComponentByClass<UAbilitySystemComponent>()))
			{
				InstigatorASC = YASC;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GA_Bleed] Activated on %s | DamagePerSecond=%.1f"),
		*GetNameSafe(ActorInfo->AvatarActor.Get()), DamagePerSecond);

	// 启动定时扣血 Tick（首次延迟一个间隔，与蓝图 WaitDelay 行为一致）
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BleedTimerHandle,
			this,
			&UGA_Bleed::BleedTick,
			BleedTickInterval,
			true,             // bLoop
			BleedTickInterval // 首次延迟 = 间隔
		);
	}
}

void UGA_Bleed::BleedTick()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	// 每 Tick 扣血 = DamagePerSecond × BleedTickInterval（帧率无关）
	// DamagePerSecond 由 FA 的 Send Gameplay Event.Magnitude 在激活时传入
	const float DamagePerTick = DamagePerSecond * BleedTickInterval;

	if (DamagePerTick > 0.f)
	{
		ASC->ApplyModToAttributeUnsafe(
			UBaseAttributeSet::GetHealthAttribute(),
			EGameplayModOp::Additive,
			-DamagePerTick);

		UE_LOG(LogTemp, Verbose, TEXT("[GA_Bleed] Tick: DPS=%.1f Damage=%.3f"),
			DamagePerSecond, DamagePerTick);

		// 流血伤害日志：转发给发起人（玩家）
		if (InstigatorASC.IsValid())
		{
			AActor* Target = GetAvatarActorFromActorInfo();
			InstigatorASC->LogDamageDealt(Target, DamagePerTick, FName("Bleed"));
		}
	}
}

void UGA_Bleed::OnBleedingTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// Tag 归零（FA 停止 / GrantTag 到期）→ 结束 GA
	if (NewCount <= 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[GA_Bleed] Bleeding Tag removed, ending GA on %s"),
			*GetNameSafe(CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Bleed::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 停止 Tick 计时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BleedTimerHandle);
	}

	// 注销 Tag 监听
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FGameplayTag BleedingTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Bleeding"));
		ActorInfo->AbilitySystemComponent->RegisterGameplayTagEvent(
			BleedingTag, EGameplayTagEventType::NewOrRemoved).Remove(TagChangeDelegateHandle);
	}

	UE_LOG(LogTemp, Log, TEXT("[GA_Bleed] EndAbility on %s"),
		*GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr));

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
