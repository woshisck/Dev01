#include "AbilitySystem/Abilities/GA_PlayerDash.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Component/SkillChargeComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

// ECC_GameTraceChannel1 = DashTrace（用于障碍 SphereTrace）
static const ECollisionChannel DashTraceChannel   = ECC_GameTraceChannel1;
// ECC_GameTraceChannel3 = Enemy（冲刺期间 Overlap）
static const ECollisionChannel EnemyChannel       = ECC_GameTraceChannel3;
// ECC_GameTraceChannel5 = DashThrough（可穿越薄墙/家具，冲刺期间 Overlap）
static const ECollisionChannel DashThroughChannel = ECC_GameTraceChannel5;

// 充能系统注册键（与 PlayerCharacterBase::BeginPlay 的 RegisterSkill 保持一致）
static const FName DashChargeTagName = TEXT("PlayerState.AbilityCast.Dash");

UGA_PlayerDash::UGA_PlayerDash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UGA_PlayerDash::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 检查充能次数（SkillChargeComponent 管理多段冲刺）
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(ActorInfo->AvatarActor.Get()))
	{
		if (Player->SkillChargeComponent &&
			!Player->SkillChargeComponent->HasCharge(FGameplayTag::RequestGameplayTag(DashChargeTagName)))
		{
			return false;
		}
	}

	return true;
}

void UGA_PlayerDash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(ActorInfo->AvatarActor.Get());
	ACharacter* Character = Player; // PlayerCharacterBase 继承自 ACharacter

	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// ── 1. 消耗充能次数（替代 CommitAbility，走 SkillChargeComponent）────────
	if (Player && Player->SkillChargeComponent)
	{
		Player->SkillChargeComponent->ConsumeCharge(
			FGameplayTag::RequestGameplayTag(DashChargeTagName));
	}

	// ── 2. 从 AbilityDA 读取蒙太奇（和 GA_MeleeAttack 一致的数据驱动方式）─────
	UCharacterDataComponent* CDC = Player ? Player->GetCharacterDataComponent() : nullptr;
	UCharacterData* CD = CDC ? CDC->GetCharacterData() : nullptr;

	FGameplayTag FirstTag;
	for (const FGameplayTag& Tag : AbilityTags) { FirstTag = Tag; break; }

	UAnimMontage* DashMontage = (CD && CD->AbilityData && FirstTag.IsValid())
		? CD->AbilityData->GetMontage(FirstTag) : nullptr;

	if (!DashMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_PlayerDash] No montage found in AbilityDA for %s — ended immediately."), *FirstTag.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// ── 3. 确定冲刺方向（Controller 已在激活前旋转好角色，直接用 ForwardVector）─
	const FVector DashDirection = Character->GetActorForwardVector();

	// ── 4. 计算实际冲刺距离（越障检测：终点延伸逐步法）──────────────────────
	const FVector StartLocation = Character->GetActorLocation();
	DashDebugStartLocation = StartLocation; // 存起来，EndAbility 时画线用

	const FVector DashEnd    = StartLocation + DashDirection * DashMaxDistance;
	const float EffectiveDist = GetFurthestValidDashDistance(StartLocation, DashEnd);
	const float AnimScale     = EffectiveDist / FMath::Max(DashMontageRootMotionLength, 1.f);

	// ── 5. 修改碰撞：穿敌人 + 穿可穿越几何（全程无刚体阻挡，由根运动驱动）───
	SetDashCollision(Character, ECR_Overlap);

	// ── 6. 播放冲刺蒙太奇（AnimRootMotionTranslationScale 控制位移距离）───────
	// AnimScale > 1 时根运动超出满距，Capsule Overlap 使玩家穿越障碍落在背后
	UYogAbilityTask_PlayMontageAndWaitForEvent* Task =
		UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
			this,
			NAME_None,
			DashMontage,
			FGameplayTagContainer(), // 不监听 GameplayEvent，纯蒙太奇等待
			1.0f,
			NAME_None,
			true,
			AnimScale);

	Task->OnCompleted.AddDynamic(this, &UGA_PlayerDash::OnMontageCompleted);
	Task->OnBlendOut.AddDynamic(this, &UGA_PlayerDash::OnMontageBlendOut);
	Task->OnInterrupted.AddDynamic(this, &UGA_PlayerDash::OnMontageInterrupted);
	Task->OnCancelled.AddDynamic(this, &UGA_PlayerDash::OnMontageCancelled);

	Task->ReadyForActivation();
}

void UGA_PlayerDash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ACharacter* Character = Cast<ACharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);

	// 恢复碰撞（必须在 Super 之前，此时 ActorInfo 仍有效）
	if (Character)
	{
		SetDashCollision(Character, ECR_Block);
	}

	// DEBUG: 蒙太奇结束后画线，此时根运动已完成，终点是真实位置
	if (Character && DashDebugStartLocation != FVector::ZeroVector)
	{
		const FVector EndLocation = Character->GetActorLocation();
		const float TotalDist     = FVector::Dist2D(DashDebugStartLocation, EndLocation);

		if (UWorld* World = GetWorld())
		{
			DrawDebugLine(World, DashDebugStartLocation, EndLocation, FColor::Cyan, false, 5.f, 0, 3.f);
			DrawDebugSphere(World, DashDebugStartLocation, 12.f, 8, FColor::Green, false, 5.f);
			DrawDebugSphere(World, EndLocation,            12.f, 8, FColor::Red,   false, 5.f);
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
				FString::Printf(TEXT("[Dash] 总位移: %.0f UU (≈%.1f m) | 配置最大: %.0f UU"),
					TotalDist, TotalDist * 0.01f, DashMaxDistance));
		}
		DashDebugStartLocation = FVector::ZeroVector;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

float UGA_PlayerDash::GetFurthestValidDashDistance(const FVector& Start, const FVector& End) const
{
	UWorld* World = GetWorld();
	if (!World) return DashMaxDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DashTrace), false);
	if (AActor* Owner = GetOwningActorFromActorInfo())
	{
		Params.AddIgnoredActor(Owner);
	}
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(DashCapsuleRadius);

	// ── 1. 前向扫描：Start → End（满距终点）─────────────────────────────────
	FHitResult ForwardHit;
	const bool bHit = World->SweepSingleByChannel(
		ForwardHit, Start, End, FQuat::Identity, DashTraceChannel, Sphere, Params);

	if (!bHit)
	{
		// 无障碍，满距冲刺
		return DashMaxDistance;
	}

	const float HitDist   = ForwardHit.Distance;
	const float Threshold = DashMaxDistance - 2.f * DashCapsuleRadius;

	// ── 2. 命中在终点附近（硬墙在末端）→ 停在障碍前 ─────────────────────────
	if (HitDist > Threshold)
	{
		return HitDist;
	}

	// ── 3. 命中在前段 → 从满距终点逐步向前延伸，寻找越障落点 ──────────────────
	// ForwardHit.TraceEnd = 传入的 End（UE5 中 TraceEnd 存储完整 End 参数，非命中点）
	const FVector DashDir   = (End - Start).GetSafeNormal();
	const FVector StepOrigin = ForwardHit.TraceEnd; // = End（满距终点）

	constexpr int32 StepCount = 6;
	constexpr float StepSize  = 50.f;

	for (int32 i = 1; i <= StepCount; ++i)
	{
		const FVector SegStart = StepOrigin + DashDir * (StepSize * (i - 1));
		const FVector SegEnd   = StepOrigin + DashDir * (StepSize * i);

		TArray<FHitResult> StepHits;
		World->SweepMultiByChannel(
			StepHits, SegStart, SegEnd, FQuat::Identity, DashTraceChannel, Sphere, Params);

		if (IsValidDashLocation(StepHits))
		{
			// 找到无阻挡落点，返回从起点到该落点的距离（> DashMaxDistance）
			return FVector::Dist(Start, SegEnd);
		}
	}

	// ── 4. 6 步内无有效落点 → 停在最初障碍前 ─────────────────────────────────
	return HitDist;
}

bool UGA_PlayerDash::IsValidDashLocation(const TArray<FHitResult>& Hits) const
{
	if (Hits.IsEmpty()) return true;

	for (const FHitResult& Hit : Hits)
	{
		UPrimitiveComponent* Comp = Hit.GetComponent();
		if (Comp && Comp->GetCollisionResponseToChannel(DashTraceChannel) == ECR_Block)
		{
			return false;
		}
	}
	return true;
}

void UGA_PlayerDash::SetDashCollision(ACharacter* Character, ECollisionResponse Response) const
{
	UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!Capsule) return;

	Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic,   Response);
	Capsule->SetCollisionResponseToChannel(ECC_Pawn,           Response);
	Capsule->SetCollisionResponseToChannel(EnemyChannel,       Response);
	Capsule->SetCollisionResponseToChannel(DashThroughChannel, Response);
}

void UGA_PlayerDash::OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayerDash::OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayerDash::OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_PlayerDash::OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_PlayerDash::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	// 每 0.5s 打印冲刺充能/CD 调试信息
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(DebugPrintTimer, this, &UGA_PlayerDash::PrintDashDebugInfo, 0.5f, true);
	}
}

void UGA_PlayerDash::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DebugPrintTimer);
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

void UGA_PlayerDash::PrintDashDebugInfo()
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningActorFromActorInfo());
	if (!Player || !Player->SkillChargeComponent) return;

	const FGameplayTag DashTag  = FGameplayTag::RequestGameplayTag(DashChargeTagName);
	const int32 CurCharge       = Player->SkillChargeComponent->GetCurrentCharge(DashTag);
	const int32 MaxCharge       = Player->SkillChargeComponent->GetMaxCharge(DashTag);
	const float CDRemaining     = Player->SkillChargeComponent->GetCDRemaining(DashTag);

	const FString Msg = (CDRemaining > 0.f)
		? FString::Printf(TEXT("冲刺: %d/%d 次 | CD: %.1fs"), CurCharge, MaxCharge, CDRemaining)
		: FString::Printf(TEXT("冲刺: %d/%d 次 | CD: 就绪"),  CurCharge, MaxCharge);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(200, 0.6f, FColor::Cyan, Msg);
	}
}
