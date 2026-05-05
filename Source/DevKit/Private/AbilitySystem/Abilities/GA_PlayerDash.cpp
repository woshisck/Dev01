#include "AbilitySystem/Abilities/GA_PlayerDash.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Camera/YogPlayerCameraManager.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Component/ComboRuntimeComponent.h"
#include "Component/SacrificeRuneComponent.h"
#include "Component/SkillChargeComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Map/AirWall.h"

static TAutoConsoleVariable<int32> CVarDashDebugTrace(
	TEXT("Dash.DebugTrace"),
	0,
	TEXT("1 = 绘制冲刺扫描路径和命中信息"));

// ECC_GameTraceChannel1 = DashTrace（用于障碍 SphereTrace）
static const ECollisionChannel DashTraceChannel   = ECC_GameTraceChannel1;
// ECC_GameTraceChannel3 = Enemy（冲刺期间 Overlap）
static const ECollisionChannel EnemyChannel       = ECC_GameTraceChannel3;
// ECC_GameTraceChannel5 = DashThrough（可穿越薄墙/家具，冲刺期间 Overlap）
static const ECollisionChannel DashThroughChannel = ECC_GameTraceChannel5;

namespace
{
	constexpr float DashStopPadding = 10.f;
	constexpr int32 MaxDashAirWallPasses = 8;

	FGameplayTag DashChargeTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("PlayerState.AbilityCast.Dash")));
	}

	bool IsEnemyOrPawnDashHit(const FHitResult& Hit)
	{
		if (Cast<APawn>(Hit.GetActor()))
		{
			return true;
		}

		const UPrimitiveComponent* Comp = Hit.GetComponent();
		return Comp && (Comp->GetCollisionObjectType() == ECC_Pawn ||
			Comp->GetCollisionObjectType() == EnemyChannel);
	}

	bool IsDashTraceBlockingHit(const FHitResult& Hit)
	{
		const UPrimitiveComponent* Comp = Hit.GetComponent();
		if (!Comp || !Hit.GetActor() || IsEnemyOrPawnDashHit(Hit))
		{
			return false;
		}

		return Hit.bBlockingHit || Comp->GetCollisionResponseToChannel(DashTraceChannel) == ECR_Block;
	}

	bool RayAabbDistanceRange(const FBox& Box, const FVector& RayStart, const FVector& RayDir, float& OutEnter, float& OutExit)
	{
		float TMin = -BIG_NUMBER;
		float TMax = BIG_NUMBER;

		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			const float StartValue = RayStart[Axis];
			const float DirValue = RayDir[Axis];
			const float MinValue = Box.Min[Axis];
			const float MaxValue = Box.Max[Axis];

			if (FMath::Abs(DirValue) <= KINDA_SMALL_NUMBER)
			{
				if (StartValue < MinValue || StartValue > MaxValue)
				{
					return false;
				}
				continue;
			}

			float T1 = (MinValue - StartValue) / DirValue;
			float T2 = (MaxValue - StartValue) / DirValue;
			if (T1 > T2)
			{
				Swap(T1, T2);
			}

			TMin = FMath::Max(TMin, T1);
			TMax = FMath::Min(TMax, T2);
			if (TMin > TMax)
			{
				return false;
			}
		}

		if (TMax < 0.f)
		{
			return false;
		}

		OutEnter = FMath::Max(0.f, TMin);
		OutExit = FMath::Max(0.f, TMax);
		return true;
	}
}

UGA_PlayerDash::UGA_PlayerDash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 受击硬直 / 击退期间不允许冲刺
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Status.HitReact"));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Status.Knockback"));
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
			!Player->SkillChargeComponent->HasCharge(DashChargeTag()))
		{
			return false;
		}
	}

	// ── 连招桥接检测 ─────────────────────────────────────────────────────
	// 蒙太奇在"桥接窗口"用 AnimNotifyState 授予 ComboSavePoint Tag。
	// 此时 CancelAbilitiesWithTag 尚未执行，ActivationOwnedTags 仍在 ASC 上，
	// 直接收集当前所有连招进度 Tag 写入 PendingSaveComboTags。
	PendingSaveComboTags.Reset();
	if (const APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(ActorInfo->AvatarActor.Get()))
	{
		if (Player->ComboRuntimeComponent && Player->ComboRuntimeComponent->HasComboSource())
		{
			return true;
		}
	}

	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		static const FGameplayTag SavePoint =
			FGameplayTag::RequestGameplayTag(TEXT("Action.Combo.DashSavePoint"), false);

		if (SavePoint.IsValid() && ASC->HasMatchingGameplayTag(SavePoint))
		{
			static const FGameplayTag CanCombo =
				FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"), false);

			// 所有已知连招进度 Tag（ActivationOwnedTags 已注入到 ASC，直接查）
			static const FName KnownComboTagNames[] = {
				TEXT("PlayerState.AbilityCast.LightAtk.Combo1"),
				TEXT("PlayerState.AbilityCast.LightAtk.Combo2"),
				TEXT("PlayerState.AbilityCast.LightAtk.Combo3"),
				TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1"),
				TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2"),
				TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3"),
			};

			PendingSaveComboTags.AddTag(CanCombo);
			for (const FName& TagName : KnownComboTagNames)
			{
				FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
				if (Tag.IsValid() && ASC->HasMatchingGameplayTag(Tag))
					PendingSaveComboTags.AddTag(Tag);
			}

			UE_LOG(LogTemp, Log, TEXT("[DashSave] ComboSavePoint detected, cached %d tags"),
				PendingSaveComboTags.Num());
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
			DashChargeTag());
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
	LastDashDirection = DashDirection.GetSafeNormal2D();

	// ── 4. 计算实际冲刺距离（越障检测：终点延伸逐步法）──────────────────────
	const FVector StartLocation = Character->GetActorLocation();
	DashDebugStartLocation = StartLocation; // 存起来，EndAbility 时画线用

	const FVector DashEnd    = StartLocation + DashDirection * DashMaxDistance;
	const float EffectiveDist = GetFurthestValidDashDistance(StartLocation, DashEnd);
	const float AnimScale     = EffectiveDist / FMath::Max(DashMontageRootMotionLength, 1.f);
	DashAnimScale = AnimScale; // 记录供 EndAbility Z 下沉诊断

	// ── 5. 修改碰撞：穿敌人 + 穿可穿越几何（全程无刚体阻挡，由根运动驱动）───
	ApplyDashMoveIgnores(Character);
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

	// 广播 Ability.Event.Dash 给玩家（BGC 事件驱动型符文监听此事件）
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		static const FGameplayTag DashEventTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Dash"));
		FGameplayEventData DashPayload;
		DashPayload.Instigator = Character;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, DashEventTag, DashPayload);
	}

	// ── 7. 通知相机进入冲刺模式（1:1 无延迟跟随）────────────────────────────
	if (Player && Player->SacrificeRuneComponent)
	{
		Player->SacrificeRuneComponent->NotifyDashExecuted(
			StartLocation,
			StartLocation + DashDirection * EffectiveDist,
			DashDirection);
	}

	if (AYogPlayerCameraManager* CM = Cast<AYogPlayerCameraManager>(
		UGameplayStatics::GetPlayerCameraManager(this, 0)))
	{
		CM->SetDashMode(true);
	}
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
		ResolveEnemyOverlapAfterDash(Character);
		SetDashCollision(Character, ECR_Block);
	}

	// 退出相机冲刺模式，恢复正常跟随
	if (AYogPlayerCameraManager* CM = Cast<AYogPlayerCameraManager>(
		UGameplayStatics::GetPlayerCameraManager(this, 0)))
	{
		CM->SetDashMode(false);
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

		// ── Z 下沉诊断 ──────────────────────────────────────────────────────
		const float ZDelta = EndLocation.Z - DashDebugStartLocation.Z;
		UE_LOG(LogTemp, Warning, TEXT("[Dash] Z移位=%.1f cm | AnimScale=%.3f | StartZ=%.1f → EndZ=%.1f"),
			ZDelta, DashAnimScale, DashDebugStartLocation.Z, EndLocation.Z);
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.f, ZDelta < -10.f ? FColor::Red : FColor::White,
				FString::Printf(TEXT("[Dash] Z移位: %.1fcm  Scale=%.2f"), ZDelta, DashAnimScale));

		// ── 地板吸附修正：若 Z 明显下沉（>10cm）强制归位 ──────────────────
		if (ZDelta < -10.f)
		{
			const FVector CheckStart = EndLocation + FVector(0.f, 0.f, 200.f);
			const FVector CheckEnd   = EndLocation - FVector(0.f, 0.f, 500.f);
			FCollisionQueryParams SnapParams(SCENE_QUERY_STAT(DashFloorCorrect), false);
			SnapParams.AddIgnoredActor(Character);
			const FCollisionShape SnapSphere = FCollisionShape::MakeSphere(25.f);

			FHitResult FloorHit;
			bool bFoundFloor = GetWorld()->SweepSingleByChannel(
				FloorHit, CheckStart, CheckEnd, FQuat::Identity, ECC_WorldStatic, SnapSphere, SnapParams);
			if (!bFoundFloor)
				bFoundFloor = GetWorld()->SweepSingleByChannel(
					FloorHit, CheckStart, CheckEnd, FQuat::Identity, ECC_WorldDynamic, SnapSphere, SnapParams);

			if (bFoundFloor)
			{
				const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
				const FVector Corrected = FVector(EndLocation.X, EndLocation.Y, FloorHit.ImpactPoint.Z + HalfHeight);
				Character->SetActorLocation(Corrected, false, nullptr, ETeleportType::TeleportPhysics);
				UE_LOG(LogTemp, Warning, TEXT("[Dash] FloorCorrect: Z %.1f → %.1f (地板 ImpactZ=%.1f)"),
					EndLocation.Z, Corrected.Z, FloorHit.ImpactPoint.Z);
				if (GEngine)
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,
						FString::Printf(TEXT("[Dash] 已修正地板 Z: %.1f → %.1f"), EndLocation.Z, Corrected.Z));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[Dash] FloorCorrect: Z下沉但未找到地板，跳过修正"));
			}
		}

		DashDebugStartLocation = FVector::ZeroVector;
	}

	if (Character)
	{
		ClearDashMoveIgnores(Character);
	}
	LastDashDirection = FVector::ZeroVector;

	// ── 连招保存：从桥接位冲刺时为下一击注入 LooseGameplayTags ─────────────
	if (!bWasCancelled && !PendingSaveComboTags.IsEmpty())
	{
		if (UYogAbilitySystemComponent* YASC = Cast<UYogAbilitySystemComponent>(
			ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr))
		{
			YASC->ApplyDashSave(PendingSaveComboTags);
		}
		PendingSaveComboTags.Reset();
	}

	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(Character))
	{
		if (Player->ComboRuntimeComponent)
		{
			Player->ComboRuntimeComponent->NotifyDashEnded(bWasCancelled);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

float UGA_PlayerDash::GetFurthestValidDashDistance(const FVector& Start, const FVector& End)
{
	UWorld* World = GetWorld();
		if (!World) return DashMaxDistance;

		DashIgnoredActors.Reset();

		float StepOffset = 0.f;
		if (ACharacter* Owner = Cast<ACharacter>(GetOwningActorFromActorInfo()))
		{
			if (UCharacterMovementComponent* CMC = Owner->GetCharacterMovement())
			{
				StepOffset = CMC->MaxStepHeight;
			}
		}

		const FVector SweepStart = Start + FVector(0.f, 0.f, StepOffset);
		const FVector SweepEnd = End + FVector(0.f, 0.f, StepOffset);
		const FVector DashDir = (SweepEnd - SweepStart).GetSafeNormal();
		const float MaxDistance = FMath::Max(0.f, FVector::Dist2D(Start, End));
		if (MaxDistance <= KINDA_SMALL_NUMBER || DashDir.IsNearlyZero())
		{
			return 0.f;
		}

		FCollisionQueryParams Params(SCENE_QUERY_STAT(DashTrace), false);
		if (AActor* Owner = GetOwningActorFromActorInfo())
		{
			Params.AddIgnoredActor(Owner);
		}

		const FCollisionShape Sphere = FCollisionShape::MakeSphere(DashCapsuleRadius);
		const bool bDebugTrace = CVarDashDebugTrace.GetValueOnGameThread() != 0;

		for (int32 Pass = 0; Pass < MaxDashAirWallPasses; ++Pass)
		{
			FHitResult BlockingHit;
			const bool bHasBlockingHit = FindFirstDashBlockingHit(SweepStart, SweepEnd, Sphere, Params, BlockingHit);
			if (bDebugTrace)
			{
				DrawDebugLine(World, SweepStart, bHasBlockingHit ? BlockingHit.ImpactPoint : SweepEnd,
					bHasBlockingHit ? FColor::Red : FColor::Green, false, 3.f, 0, 2.f);
				DrawDebugSphere(World, SweepStart, DashCapsuleRadius, 12, FColor::Yellow, false, 3.f);
			}

			if (!bHasBlockingHit)
			{
				return MaxDistance;
			}

			AActor* HitActor = BlockingHit.GetActor();
			UPrimitiveComponent* HitComp = BlockingHit.GetComponent();
			const float StopDistance = GetDashStopDistance(BlockingHit.Distance);

			if (bDebugTrace)
			{
				DrawDebugSphere(World, BlockingHit.ImpactPoint, DashCapsuleRadius, 12, FColor::Red, false, 3.f);
				DrawDebugLine(World, BlockingHit.ImpactPoint, BlockingHit.ImpactPoint + BlockingHit.ImpactNormal * 60.f,
					FColor::White, false, 3.f, 0, 2.f);
				UE_LOG(LogTemp, Warning, TEXT("[DashTrace] Hit=%s Comp=%s Dist=%.0f Pass=%d"),
					*GetNameSafe(HitActor), *GetNameSafe(HitComp), BlockingHit.Distance, Pass);
			}

			if (!HitActor || HitActor->ActorHasTag(TEXT("DashBarrier")))
			{
				return StopDistance;
			}

			AAirWall* AirWall = Cast<AAirWall>(HitActor);
			if (!AirWall || !AirWall->bAllowDashThrough)
			{
				return StopDistance;
			}

			if (BlockingHit.Distance > MaxDistance * (2.f / 3.f))
			{
				return StopDistance;
			}

			const float ExitDistance = FindAirWallExitDistance(SweepStart, DashDir, HitComp);
			if (ExitDistance < 0.f || ExitDistance > MaxDistance)
			{
				return StopDistance;
			}

			DashIgnoredActors.AddUnique(AirWall);
			Params.AddIgnoredActor(AirWall);
		}

	return MaxDistance;

}

bool UGA_PlayerDash::FindFirstDashBlockingHit(
	const FVector& SweepStart,
	const FVector& SweepEnd,
	const FCollisionShape& Shape,
	const FCollisionQueryParams& Params,
	FHitResult& OutHit) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(Hits, SweepStart, SweepEnd, FQuat::Identity, DashTraceChannel, Shape, Params);
	Hits.Sort([](const FHitResult& A, const FHitResult& B)
	{
		return A.Distance < B.Distance;
	});

	for (const FHitResult& Hit : Hits)
	{
		if (IsDashTraceBlockingHit(Hit))
		{
			OutHit = Hit;
			return true;
		}
	}

	return false;
}

float UGA_PlayerDash::FindAirWallExitDistance(
	const FVector& SweepStart,
	const FVector& DashDirection,
	const UPrimitiveComponent* AirWallComponent) const
{
	if (!AirWallComponent || DashDirection.IsNearlyZero())
	{
		return -1.f;
	}

	const FBox ExpandedBounds = AirWallComponent->Bounds.GetBox().ExpandBy(DashCapsuleRadius + DashStopPadding);
	float EnterDistance = 0.f;
	float ExitDistance = 0.f;
	if (!RayAabbDistanceRange(ExpandedBounds, SweepStart, DashDirection.GetSafeNormal(), EnterDistance, ExitDistance))
	{
		return -1.f;
	}

	return ExitDistance;
}

float UGA_PlayerDash::GetDashStopDistance(float HitDistance) const
{
	return FMath::Max(0.f, HitDistance - DashStopPadding);
}


void UGA_PlayerDash::SetDashCollision(ACharacter* Character, ECollisionResponse Response) const
{
	UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!Capsule) return;

	// 只改"允许被穿越"的通道；WorldDynamic/WorldStatic 保持不变，防止穿地板/动态关卡几何体。
	// 如需某物体可被穿越，在该 Mesh 的 Collision Profile 里手动将 DashThrough 设为 Overlap 即可。
	Capsule->SetCollisionResponseToChannel(ECC_Pawn,           Response); // 穿透其他 Pawn
	Capsule->SetCollisionResponseToChannel(EnemyChannel,       Response); // 穿透敌人（自定义通道）
	if (Response == ECR_Block)
	{
		Capsule->SetCollisionResponseToChannel(DashThroughChannel, ECR_Block);
	}
}

void UGA_PlayerDash::ApplyDashMoveIgnores(ACharacter* Character)
{
	UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!Character || !Capsule)
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& WeakActor : DashIgnoredActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			Character->MoveIgnoreActorAdd(Actor);
			Capsule->IgnoreActorWhenMoving(Actor, true);
		}
	}
}

void UGA_PlayerDash::ClearDashMoveIgnores(ACharacter* Character)
{
	UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!Character || !Capsule)
	{
		DashIgnoredActors.Reset();
		return;
	}

	for (const TWeakObjectPtr<AActor>& WeakActor : DashIgnoredActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			Character->MoveIgnoreActorRemove(Actor);
			Capsule->IgnoreActorWhenMoving(Actor, false);
		}
	}
	DashIgnoredActors.Reset();
}

bool UGA_PlayerDash::HasEnemyOverlapAt(ACharacter* Character, const FVector& Location) const
{
	UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!World || !Character || !Capsule)
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(EnemyChannel);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DashEnemyOverlap), false);
	QueryParams.AddIgnoredActor(Character);

	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
		Capsule->GetScaledCapsuleRadius(),
		Capsule->GetScaledCapsuleHalfHeight());

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Location, FQuat::Identity, ObjectParams, CapsuleShape, QueryParams);
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AActor* Actor = Overlap.GetActor();
		const UPrimitiveComponent* Comp = Overlap.GetComponent();
		if (!Actor || Actor == Character)
		{
			continue;
		}

		if (Cast<APawn>(Actor) || (Comp && Comp->GetCollisionObjectType() == EnemyChannel))
		{
			return true;
		}
	}

	return false;
}

void UGA_PlayerDash::ResolveEnemyOverlapAfterDash(ACharacter* Character) const
{
	if (!Character || !HasEnemyOverlapAt(Character, Character->GetActorLocation()))
	{
		return;
	}

	FVector BackDirection = LastDashDirection.IsNearlyZero()
		? -Character->GetActorForwardVector().GetSafeNormal2D()
		: -LastDashDirection.GetSafeNormal2D();
	if (BackDirection.IsNearlyZero())
	{
		return;
	}

	const FVector StartLocation = Character->GetActorLocation();
	constexpr int32 MaxSteps = 12;
	constexpr float StepSize = 20.f;
	for (int32 Step = 1; Step <= MaxSteps; ++Step)
	{
		const FVector Candidate = StartLocation + BackDirection * (StepSize * Step);
		if (!HasEnemyOverlapAt(Character, Candidate))
		{
			FHitResult MoveHit;
			Character->SetActorLocation(Candidate, true, &MoveHit, ETeleportType::TeleportPhysics);
			if (!HasEnemyOverlapAt(Character, Character->GetActorLocation()))
			{
				UE_LOG(LogTemp, Warning, TEXT("[Dash] EnemyOverlapCorrect: moved back %.0f cm"), StepSize * Step);
				return;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Dash] EnemyOverlapCorrect: no safe backward point found"));
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

	// 每 0.1s 打印冲刺充能/CD 调试信息
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(DebugPrintTimer, this, &UGA_PlayerDash::PrintDashDebugInfo, 0.1f, true);
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

	const FGameplayTag DashTag  = DashChargeTag();
	const int32 CurCharge       = Player->SkillChargeComponent->GetCurrentCharge(DashTag);
	const int32 MaxCharge       = Player->SkillChargeComponent->GetMaxCharge(DashTag);
	const float CDRemaining     = Player->SkillChargeComponent->GetCDRemaining(DashTag);

	const FString Msg = (CDRemaining > 0.f)
		? FString::Printf(TEXT("冲刺: %d/%d 次 | CD: %.1fs"), CurCharge, MaxCharge, CDRemaining)
		: FString::Printf(TEXT("冲刺: %d/%d 次 | CD: 就绪"),  CurCharge, MaxCharge);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(200, 0.15f, FColor::Cyan, Msg);
	}
}
