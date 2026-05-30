// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AN_MeleeDamage.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/Abilities/GA_PlayMontage.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/GameplayEffect/GE_MeleeAttackFrame.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimInstance.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

// HitSuccessDilation is temporarily disabled. Keep this file-local code around as a comment
// so it can be restored quickly if the impact feedback direction changes again.
// namespace
// {
// 	void RestoreGlobalHitSuccessDilation(TWeakObjectPtr<UWorld> WeakWorld)
// 	{
// 		if (UWorld* World = WeakWorld.Get())
// 		{
// 			UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
// 		}
// 	}
//
// 	void RestoreSelfHitSuccessDilation(TWeakObjectPtr<AActor> WeakActor)
// 	{
// 		if (AActor* Actor = WeakActor.Get())
// 		{
// 			Actor->CustomTimeDilation = 1.f;
// 		}
// 	}
// }

#if WITH_EDITOR && ENABLE_DRAW_DEBUG
namespace
{
	void AN_MeleeDamage_DrawPersonaHitbox(UWorld* World, const FVector& Loc, float Yaw, const FActionData& Data)
	{
		if (!World) return;

		const float OuterR = Data.ActRange > 0.f ? Data.ActRange : 400.f;
		const FColor Color = FColor::Cyan;
		constexpr float Duration = 2.f;

		if (Data.hitboxTypes.IsEmpty())
		{
			constexpr int32 Seg = 32;
			FVector Prev = Loc + FVector(OuterR, 0, 0);
			for (int32 i = 1; i <= Seg; ++i)
			{
				float Rad = FMath::DegreesToRadians(360.f * i / Seg);
				FVector Cur = Loc + FVector(FMath::Cos(Rad) * OuterR, FMath::Sin(Rad) * OuterR, 0);
				DrawDebugLine(World, Prev, Cur, Color, false, Duration);
				Prev = Cur;
			}
			return;
		}

		for (const FYogHitboxType& HB : Data.hitboxTypes)
		{
			if (HB.hitboxType == EHitBoxType::Annulus)
			{
				const FHitboxAnnulus& Ann = HB.AnnulusHitbox;
				const float InnerR  = Ann.inner_radius;
				const float HalfDeg = Ann.degree * 0.5f;
				const float StartDeg = Yaw - HalfDeg;
				const float EndDeg   = Yaw + HalfDeg;

				const float EffectiveOffset = Ann.bAutoOffset ? -InnerR : Ann.OffsetCore;
				const float YawRad = FMath::DegreesToRadians(Yaw);
				const FVector CenterLoc = Loc + FVector(FMath::Cos(YawRad), FMath::Sin(YawRad), 0.f) * EffectiveOffset;
				const float EffectiveOuterR = (Ann.bAutoOffset && InnerR > 0.f) ? OuterR + InnerR : OuterR;

				constexpr int32 Seg = 24;
				FVector PrevOuter = FVector::ZeroVector;
				FVector PrevInner = FVector::ZeroVector;

				for (int32 i = 0; i <= Seg; ++i)
				{
					float Deg = FMath::Lerp(StartDeg, EndDeg, (float)i / Seg);
					float Rad = FMath::DegreesToRadians(Deg);
					FVector Outer = CenterLoc + FVector(FMath::Cos(Rad) * EffectiveOuterR, FMath::Sin(Rad) * EffectiveOuterR, 0);
					FVector Inner = CenterLoc + FVector(FMath::Cos(Rad) * InnerR, FMath::Sin(Rad) * InnerR, 0);
					if (i > 0)
					{
						DrawDebugLine(World, PrevOuter, Outer, Color, false, Duration);
						if (InnerR > 0) DrawDebugLine(World, PrevInner, Inner, Color, false, Duration);
					}
					PrevOuter = Outer; PrevInner = Inner;
				}

				auto RadEdge = [&](float Deg, float R)
				{
					float Rad = FMath::DegreesToRadians(Deg);
					return CenterLoc + FVector(FMath::Cos(Rad) * R, FMath::Sin(Rad) * R, 0);
				};
				DrawDebugLine(World, InnerR > 0 ? RadEdge(StartDeg, InnerR) : CenterLoc, RadEdge(StartDeg, EffectiveOuterR), Color, false, Duration);
				DrawDebugLine(World, InnerR > 0 ? RadEdge(EndDeg,   InnerR) : CenterLoc, RadEdge(EndDeg,   EffectiveOuterR), Color, false, Duration);
			}
			else if (HB.hitboxType == EHitBoxType::Triangle)
			{
				const TArray<FHitboxTriangle>& Tri = HB.HitboxTriangles;
				for (int32 i = 0; i < Tri.Num(); ++i)
				{
					float Rad = FMath::DegreesToRadians(Yaw + Tri[i].Degree);
					FVector Pt = Loc + FVector(FMath::Cos(Rad) * OuterR, FMath::Sin(Rad) * OuterR, 0);
					DrawDebugLine(World, Loc, Pt, Color, false, Duration);
					if (i > 0)
					{
						float PrevRad = FMath::DegreesToRadians(Yaw + Tri[i - 1].Degree);
						FVector PrevPt = Loc + FVector(FMath::Cos(PrevRad) * OuterR, FMath::Sin(PrevRad) * OuterR, 0);
						DrawDebugLine(World, PrevPt, Pt, Color, false, Duration);
					}
				}
			}
			else if (HB.hitboxType == EHitBoxType::Square)
			{
				// Local Y (right) = perpendicular to forward, 90° CCW: (-Sin, Cos, 0)
				const float YawRad   = FMath::DegreesToRadians(Yaw);
				const FVector LocalY = FVector(-FMath::Sin(YawRad), FMath::Cos(YawRad), 0.f);
				for (const FHitboxSquare& Sq : HB.HitboxSquares)
				{
					const float HalfRange = OuterR * 0.5f;
					const float HalfW     = Sq.Width > 0.f ? Sq.Width * 0.5f : HalfRange;
					DrawDebugBox(World,
						Loc + LocalY * HalfRange,
						FVector(HalfW, HalfRange, 60.f),
						FRotator(0.f, Yaw, 0.f).Quaternion(),
						Color, false, Duration);
				}
			}
		}
	}
}
#endif // WITH_EDITOR && ENABLE_DRAW_DEBUG

void UAN_MeleeDamageMontageCleanupBinding::Initialize(UAN_MeleeDamage* InOwnerNotify, UAnimInstance* InAnimInstance,
	UAbilitySystemComponent* InASC, FActiveGameplayEffectHandle InGEHandle)
{
	OwnerNotify = InOwnerNotify;
	AnimInstance = InAnimInstance;
	ASC = InASC;
	GEHandle = InGEHandle;
}

void UAN_MeleeDamageMontageCleanupBinding::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)Montage;
	(void)bInterrupted;

	if (UAbilitySystemComponent* ValidASC = ASC.Get())
	{
		ValidASC->RemoveActiveGameplayEffect(GEHandle);
	}

	if (UAnimInstance* ValidAnimInstance = AnimInstance.Get())
	{
		ValidAnimInstance->OnMontageEnded.RemoveDynamic(this, &UAN_MeleeDamageMontageCleanupBinding::HandleMontageEnded);
	}

	if (UAN_MeleeDamage* ValidOwnerNotify = OwnerNotify.Get())
	{
		ValidOwnerNotify->RemoveCleanupBinding(this);
	}
}

UAN_MeleeDamage::UAN_MeleeDamage()
{
	EventTag = FGameplayTag::RequestGameplayTag(FName("GameplayEffect.DamageType.GeneralAttack"));
}

#if WITH_EDITOR
bool UAN_MeleeDamage::ShouldFireInEditor()
{
	return bDrawDebugHitbox;
}
#endif

void UAN_MeleeDamage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

#if WITH_EDITOR && ENABLE_DRAW_DEBUG
	if (MeshComp && bDrawDebugHitbox)
	{
		UWorld* DebugWorld = MeshComp->GetWorld();
		if (DebugWorld && DebugWorld->WorldType == EWorldType::EditorPreview)
		{
			const AActor* DebugOwner = MeshComp->GetOwner();
			const float DebugYaw = DebugOwner ? DebugOwner->GetActorRotation().Yaw : 0.f;
			AN_MeleeDamage_DrawPersonaHitbox(DebugWorld, MeshComp->GetComponentLocation(), DebugYaw, BuildActionData());
			return;
		}
	}
#endif

	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	AYogCharacterBase* Character = Cast<AYogCharacterBase>(Owner);
	if (!Character) return;

	const UMontageAttackDataAsset* EffectiveAttackData = AttackDataOverride;
	const FComboAttackConfig* EffectiveNodeAttackConfig = nullptr;
	if (UYogAbilitySystemComponent* ASC = Character->GetASC())
	{
		if (const UGA_MeleeAttack* MeleeGA = Cast<UGA_MeleeAttack>(ASC->GetCurrentAbilityInstance()))
		{
			if (const FComboAttackConfig* NodeAttackConfig = MeleeGA->GetConfiguredAttackConfig())
			{
				EffectiveNodeAttackConfig = NodeAttackConfig;
			}
			else if (MeleeGA->HasConfiguredAttackData())
			{
				EffectiveAttackData = MeleeGA->GetConfiguredAttackData();
			}
		}
	}

	const TArray<TObjectPtr<URuneDataAsset>>& EffectiveRuneEffects = EffectiveNodeAttackConfig
		? EffectiveNodeAttackConfig->AdditionalRuneEffects
		: EffectiveAttackData
		? EffectiveAttackData->AdditionalRuneEffects
		: AdditionalRuneEffects;

	// 将附加 Rune 暂存到角色，GA_MeleeAttack::OnEventReceived 会读取并触发到命中目标
	if (EffectiveRuneEffects.Num() > 0)
	{
		Character->PendingAdditionalHitRunes = EffectiveRuneEffects;
	}

	const EHitStopMode EffectiveHitStopMode = EffectiveNodeAttackConfig
		? EffectiveNodeAttackConfig->HitStopMode
		: EffectiveAttackData
		? EffectiveAttackData->HitStopMode
		: HitStopMode;
	if (EffectiveHitStopMode != EHitStopMode::None)
	{
		auto& Override = Character->PendingHitStopOverride;
		Override.bActive = true;
		Override.Mode = EffectiveHitStopMode;
		Override.FrozenDuration = EffectiveNodeAttackConfig ? EffectiveNodeAttackConfig->HitStopFrozenDuration : EffectiveAttackData ? EffectiveAttackData->HitStopFrozenDuration : HitStopFrozenDuration;
		Override.SlowDuration = EffectiveNodeAttackConfig ? EffectiveNodeAttackConfig->HitStopSlowDuration : EffectiveAttackData ? EffectiveAttackData->HitStopSlowDuration : HitStopSlowDuration;
		Override.SlowRate = EffectiveNodeAttackConfig ? EffectiveNodeAttackConfig->HitStopSlowRate : EffectiveAttackData ? EffectiveAttackData->HitStopSlowRate : HitStopSlowRate;
		Override.CatchUpRate = EffectiveNodeAttackConfig ? EffectiveNodeAttackConfig->HitStopCatchUpRate : EffectiveAttackData ? EffectiveAttackData->HitStopCatchUpRate : HitStopCatchUpRate;
	}

	static const TArray<FGameplayTag> EmptyOnHitEventTags;
	const TArray<FGameplayTag>& EffectiveOnHitEventTags = EffectiveNodeAttackConfig
		? EffectiveNodeAttackConfig->OnHitEventTags
		: EffectiveAttackData
		? EffectiveAttackData->OnHitEventTags
		: EmptyOnHitEventTags;
	if (EffectiveOnHitEventTags.Num() > 0)
	{
		Character->PendingOnHitEventTags = EffectiveOnHitEventTags;
	}

	if (HitImpactCueTag.IsValid())
	{
		Character->PendingHitImpactCueTag = HitImpactCueTag;
	}

	const FActionData EffectiveActionData = EffectiveNodeAttackConfig
		? EffectiveNodeAttackConfig->BuildActionData()
		: EffectiveAttackData
		? EffectiveAttackData->BuildActionData()
		: BuildActionData();

	if (UYogAbilitySystemComponent* ASC = Character->GetASC())
	{
		ApplyAttackFrameGE(Character, MeshComp, ASC, EffectiveActionData);
	}

	FGameplayEventData EventData;
	EventData.Instigator   = Character;
	EventData.EventTag     = EffectiveNodeAttackConfig && EffectiveNodeAttackConfig->EventTag.IsValid()
		? EffectiveNodeAttackConfig->EventTag
		: EffectiveAttackData && EffectiveAttackData->EventTag.IsValid()
		? EffectiveAttackData->EventTag
		: EventTag;
	// 将自身作为 OptionalObject 传递，TargetType 通过它直接读取 HitboxTypes / ActRange 等参数
	EventData.OptionalObject = this;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventData.EventTag, EventData);

	// 挥刀模式刀光符文：每次攻击帧均发送 Action.Attack.Swing，供 GA_SlashWaveCounter 计数
	// （无论是否命中敌人，只要动画播放到攻击帧就触发）
	static const FGameplayTag TAG_Swing =
		FGameplayTag::RequestGameplayTag(FName("Action.Attack.Swing"));
	FGameplayEventData SwingEventData;
	SwingEventData.Instigator = Character;
	SwingEventData.EventTag   = TAG_Swing;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, TAG_Swing, SwingEventData);
}

FString UAN_MeleeDamage::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Melee Dmg | %.0f / R:%.0f"), ActDamage, ActRange);
}

FActionData UAN_MeleeDamage::BuildActionData() const
{
	if (AttackDataOverride)
	{
		return AttackDataOverride->BuildActionData();
	}

	FActionData Out;
	Out.ActDamage     = ActDamage;
	Out.ActRange      = ActRange;
	Out.ActResilience = ActResilience;
	Out.ActDmgReduce  = ActDmgReduce;
	Out.hitboxTypes   = HitboxTypes;
	return Out;
}

void UAN_MeleeDamage::ApplyAttackFrameGE(AYogCharacterBase* Character, USkeletalMeshComponent* MeshComp,
	UAbilitySystemComponent* ASC, const FActionData& ActionData)
{
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Character);
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
		UGE_MeleeAttackFrame::StaticClass(), 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	static const FGameplayTag TAG_ActDamage    = FGameplayTag::RequestGameplayTag("Attribute.ActDamage");
	static const FGameplayTag TAG_ActRange     = FGameplayTag::RequestGameplayTag("Attribute.ActRange");
	static const FGameplayTag TAG_ActRes       = FGameplayTag::RequestGameplayTag("Attribute.ActResilience");
	static const FGameplayTag TAG_ActDmgReduce = FGameplayTag::RequestGameplayTag("Attribute.ActDmgReduce");

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActDamage,    ActionData.ActDamage);
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActRange,     ActionData.ActRange);
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActRes,       ActionData.ActResilience);
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActDmgReduce, ActionData.ActDmgReduce);

	const FActiveGameplayEffectHandle GEHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	if (!GEHandle.IsValid())
	{
		return;
	}

	UAnimInstance* AnimInst = MeshComp->GetAnimInstance();
	if (!AnimInst)
	{
		// No anim instance to bind end callback — remove immediately to avoid leaking the GE.
		ASC->RemoveActiveGameplayEffect(GEHandle);
		return;
	}

	// OnMontageEnded is a dynamic multicast delegate in UE 5.4, so it cannot bind
	// lambdas or be removed by delegate handle. Keep a transient UObject alive to
	// remove this exact GE when the montage ends, then unbind itself.
	UAN_MeleeDamageMontageCleanupBinding* CleanupBinding =
		NewObject<UAN_MeleeDamageMontageCleanupBinding>(this);
	CleanupBinding->Initialize(this, AnimInst, ASC, GEHandle);
	ActiveCleanupBindings.Add(CleanupBinding);
	AnimInst->OnMontageEnded.AddUniqueDynamic(CleanupBinding, &UAN_MeleeDamageMontageCleanupBinding::HandleMontageEnded);
}

void UAN_MeleeDamage::RemoveCleanupBinding(UAN_MeleeDamageMontageCleanupBinding* Binding)
{
	ActiveCleanupBindings.Remove(Binding);
}

void UAN_MeleeDamage::ApplyHitSuccessDilation(AActor* SourceActor) const
{
	// Temporarily disabled: hit freeze/slow now handles melee impact feedback.
	// if (!SourceActor || HitSuccessDilation.Scope == EMeleeDamageHitDilationScope::None || HitSuccessDilation.DurationSeconds <= 0.f)
	// {
	// 	return;
	// }
	//
	// UWorld* World = SourceActor->GetWorld();
	// if (!World)
	// {
	// 	return;
	// }
	//
	// const float Factor = FMath::Clamp(HitSuccessDilation.DilationFactor, 0.01f, 1.0f);
	// float TimerDuration = FMath::Max(HitSuccessDilation.DurationSeconds, 0.01f);
	// FTimerHandle RestoreHandle;
	//
	// if (HitSuccessDilation.Scope == EMeleeDamageHitDilationScope::Global)
	// {
	// 	UGameplayStatics::SetGlobalTimeDilation(World, Factor);
	// 	TimerDuration *= Factor;
	// 	World->GetTimerManager().SetTimer(
	// 		RestoreHandle,
	// 		FTimerDelegate::CreateStatic(&RestoreGlobalHitSuccessDilation, TWeakObjectPtr<UWorld>(World)),
	// 		FMath::Max(TimerDuration, 0.01f),
	// 		false);
	// }
	// else if (HitSuccessDilation.Scope == EMeleeDamageHitDilationScope::Self)
	// {
	// 	SourceActor->CustomTimeDilation = Factor;
	// 	World->GetTimerManager().SetTimer(
	// 		RestoreHandle,
	// 		FTimerDelegate::CreateStatic(&RestoreSelfHitSuccessDilation, TWeakObjectPtr<AActor>(SourceActor)),
	// 		TimerDuration,
	// 		false);
	// }
}
