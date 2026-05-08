#include "AbilitySystem/Abilities/GA_SlashWaveCounter.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Projectile/SlashWaveProjectile.h"

UGA_SlashWaveCounter::UGA_SlashWaveCounter()
{
	// 每个 Actor 保留一个实例，持续存在于整个角色生命周期
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 被动 GA：不需要输入绑定，不占用技能槽
	bServerRespectsRemoteAbilityCancellation = false;
}

void UGA_SlashWaveCounter::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// 被动 GA：授予时立即自我激活，等待攻击事件
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
}

void UGA_SlashWaveCounter::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 被动 GA 不消耗资源
	// CommitAbility 不需要调用

	// 确定监听的事件 Tag
	// 命中模式：FA_Rune_SlashWave 的 OnDamageDealt 发送此 Tag 到玩家 ASC
	// 挥刀模式：AN_MeleeDamage::Notify 每次挥刀时发送此 Tag 到玩家 ASC
	const FGameplayTag ListenTag = bHitRequired
		? FGameplayTag::RequestGameplayTag(FName("Action.Rune.SlashWaveHit"))
		: FGameplayTag::RequestGameplayTag(FName("Action.Attack.Swing"));

	// 创建持续监听任务（OnlyTriggerOnce=false，永久监听直到 GA 结束）
	WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ListenTag,
		nullptr,  // OptionalExternalTarget（监听自身 ASC）
		false,    // OnlyTriggerOnce：false = 持续监听
		true      // bAsyncAddToASC
	);

	WaitEventTask->EventReceived.AddDynamic(this, &UGA_SlashWaveCounter::OnAttackEventReceived);
	WaitEventTask->ReadyForActivation();

	UE_LOG(LogTemp, Log, TEXT("[GA_SlashWaveCounter] Activated, listening: %s, HitsRequired=%d"),
		*ListenTag.ToString(), HitsRequired);
}

void UGA_SlashWaveCounter::OnAttackEventReceived(FGameplayEventData Payload)
{
	// 挥刀模式守卫：仅当所有者持有 SwingModeGateTag 时才计数
	if (!bHitRequired && SwingModeGateTag.IsValid())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (!ASC || !ASC->HasMatchingGameplayTag(SwingModeGateTag))
		{
			return; // 符文未装备，忽略此次挥刀
		}
	}

	AActor* EventTarget = const_cast<AActor*>(Payload.Target.Get());
	AActor* Owner = GetAvatarActorFromActorInfo();
	if (EventTarget && EventTarget != Owner)
	{
		PendingSlashWaveInitialTarget = EventTarget;
	}
	else
	{
		PendingSlashWaveInitialTarget.Reset();
	}

	++CurrentCount;

	UE_LOG(LogTemp, Warning, TEXT("[GA_SlashWaveCounter] Event Target=%s Instigator=%s Count=%d/%d"),
		*GetNameSafe(EventTarget),
		*GetNameSafe(Payload.Instigator.Get()),
		CurrentCount,
		HitsRequired);

	if (CurrentCount >= HitsRequired)
	{
		CurrentCount = 0;
		SpawnSlashWave();
	}
}

void UGA_SlashWaveCounter::SpawnSlashWave()
{
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_SlashWaveCounter] ProjectileClass 未配置，跳过生成"));
		PendingSlashWaveInitialTarget.Reset();
		return;
	}

	ACharacter* Owner = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Owner || !GetWorld())
	{
		PendingSlashWaveInitialTarget.Reset();
		return;
	}

	// 在角色前方 SpawnOffset 处生成，旋转与角色一致（朝向由 ProjectileMovement 的初速度决定）
	const FVector Forward  = Owner->GetActorForwardVector();
	const FVector SpawnLoc = Owner->GetActorLocation() + Forward * SpawnOffset;
	const FRotator SpawnRot = Owner->GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner   = Owner;
	SpawnParams.Instigator = Owner;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform SpawnTransform(SpawnRot, SpawnLoc);
	ASlashWaveProjectile* Projectile = GetWorld()->SpawnActorDeferred<ASlashWaveProjectile>(
		ProjectileClass,
		SpawnTransform,
		SpawnParams.Owner,
		SpawnParams.Instigator,
		SpawnParams.SpawnCollisionHandlingOverride);

	if (Projectile)
	{
		Projectile->SetSourceCharacterForSpawn(Owner);
		Projectile->FinishSpawning(SpawnTransform);
		Projectile->InitProjectile(Owner, SlashDamage, SlashDamageEffect);
		Projectile->ApplyImmediateHit(PendingSlashWaveInitialTarget.Get());

		UE_LOG(LogTemp, Warning, TEXT("[GA_SlashWaveCounter] SlashWave spawned at %s InitialTarget=%s"),
			*SpawnLoc.ToString(),
			*GetNameSafe(PendingSlashWaveInitialTarget.Get()));
	}

	PendingSlashWaveInitialTarget.Reset();
}

void UGA_SlashWaveCounter::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (WaitEventTask)
	{
		WaitEventTask->EndTask();
		WaitEventTask = nullptr;
	}

	CurrentCount = 0;
	PendingSlashWaveInitialTarget.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
