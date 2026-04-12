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

	// ── 4. 计算冲刺目标（越障检测）────────────────────────────────────────────
	const FVector StartLocation = Character->GetActorLocation();
	DashDebugStartLocation = StartLocation; // 存起来，EndAbility 时画线用

	bool    bIsTraversal  = false;
	FVector TraversalEnd  = FVector::ZeroVector;
	float   AnimScale     = 1.0f;
	ComputeDashTarget(StartLocation, DashDirection, bIsTraversal, TraversalEnd, AnimScale);

	// ── 5. 修改碰撞：穿敌人 + 穿可穿越几何 ──────────────────────────────────
	SetDashCollision(Character, ECR_Overlap);

	// ── 6. 越障：瞬间传送到墙另一侧，蒙太奇仅作视觉（AnimScale=0）───────────
	if (bIsTraversal)
	{
		Character->SetActorLocation(TraversalEnd, false);
		AnimScale = 0.f;
	}
	// 非越障：完全由根运动驱动位移，不调用 SetActorLocation

	// ── 7. 播放冲刺蒙太奇（AnimRootMotionTranslationScale 控制位移距离）───────
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

void UGA_PlayerDash::ComputeDashTarget(
	const FVector& Start,
	const FVector& Direction,
	bool&    bOutIsTraversal,
	FVector& OutTraversalEnd,
	float&   OutAnimScale) const
{
	bOutIsTraversal = false;
	OutTraversalEnd = FVector::ZeroVector;
	OutAnimScale    = DashMaxDistance / FMath::Max(DashMontageRootMotionLength, 1.f);

	UWorld* World = GetWorld();
	if (!World) return;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(DashTrace), false);
	if (AActor* Owner = GetOwningActorFromActorInfo())
	{
		Params.AddIgnoredActor(Owner);
	}
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(DashCapsuleRadius);

	// ── 1. 前向扫描：找障碍物入口 ────────────────────────────────────────────
	const FVector ForwardEnd = Start + Direction * DashMaxDistance;
	FHitResult    ForwardHit;
	const bool bWallFound = World->SweepSingleByChannel(
		ForwardHit, Start, ForwardEnd, FQuat::Identity, DashTraceChannel, Sphere, Params);

	if (!bWallFound)
	{
		// 无障碍，正常满距冲刺；OutAnimScale 已设好
		return;
	}

	const float WallEntryDist = ForwardHit.Distance;
	const float HalfwayDist   = DashMaxDistance * 0.5f;
	const float Remaining     = DashMaxDistance - WallEntryDist;

	// ── 2. 后半段遇墙 / 剩余不足 → 停在墙前 ─────────────────────────────────
	if (WallEntryDist >= HalfwayDist || Remaining < 100.f)
	{
		OutAnimScale = WallEntryDist / FMath::Max(DashMontageRootMotionLength, 1.f);
		return;
	}

	// ── 3. 后向扫描：找墙另一侧出口 ──────────────────────────────────────────
	// 从（起点 + DashMax + MaxWallThick）向回扫，命中即为墙出口
	const float   BackSearchRange  = DashMaxDistance + MaxTraversableWallThickness;
	const FVector BackwardStart    = Start + Direction * BackSearchRange;
	FHitResult    BackwardHit;
	const bool bBackHit = World->SweepSingleByChannel(
		BackwardHit, BackwardStart, ForwardHit.Location,
		FQuat::Identity, DashTraceChannel, Sphere, Params);

	if (!bBackHit)
	{
		// 无法找到墙出口（墙延伸超出搜索范围）→ 停在墙前
		OutAnimScale = WallEntryDist / FMath::Max(DashMontageRootMotionLength, 1.f);
		return;
	}

	// 墙出口距起点距离 = 后向扫描起点到起点的距离 − 后向命中距离
	const float WallExitDist  = BackSearchRange - BackwardHit.Distance;
	const float WallThickness = WallExitDist - WallEntryDist;

	// ── 4. 墙太厚 → 停在墙前 ─────────────────────────────────────────────────
	if (WallThickness <= 0.f || WallThickness > MaxTraversableWallThickness)
	{
		OutAnimScale = WallEntryDist / FMath::Max(DashMontageRootMotionLength, 1.f);
		return;
	}

	// ── 5. 满足越障条件：传送到墙出口，蒙太奇视觉播放（Scale=0）───────────────
	bOutIsTraversal = true;
	OutTraversalEnd = FVector(
		(Start + Direction * WallExitDist).X,
		(Start + Direction * WallExitDist).Y,
		Start.Z);
	OutAnimScale = 0.f;
}

void UGA_PlayerDash::SetDashCollision(ACharacter* Character, ECollisionResponse Response) const
{
	UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!Capsule) return;

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
