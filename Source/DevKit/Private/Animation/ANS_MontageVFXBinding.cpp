#include "Animation/ANS_MontageVFXBinding.h"

#include "Animation/AN_MeleeDamage.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Component/MontageVFXBindingComponent.h"

namespace
{
	const UAN_MeleeDamage* FindFirstMeleeDamageNotify(const UAnimSequenceBase* Animation)
	{
		if (!Animation)
		{
			return nullptr;
		}

		for (const FAnimNotifyEvent& Event : Animation->Notifies)
		{
			if (const UAN_MeleeDamage* DamageNotify = Cast<UAN_MeleeDamage>(Event.Notify))
			{
				return DamageNotify;
			}
		}

		return nullptr;
	}
}

void UANS_MontageVFXBinding::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp || SlotName.IsNone())
	{
		return;
	}

	if (bGateMontageSectionWithRemainTime && ChargeGateRuntimeByMesh.Contains(TObjectKey<USkeletalMeshComponent>(MeshComp)))
	{
		return;
	}

	AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
	UMontageVFXBindingComponent* VFXBindingComponent = Character
		? Character->FindComponentByClass<UMontageVFXBindingComponent>()
		: nullptr;
	if (!Character || !VFXBindingComponent)
	{
		return;
	}

	FActionData ActionData;
	const FActionData* ActionDataPtr = nullptr;
	if (UYogAbilitySystemComponent* ASC = Character->GetASC())
	{
		if (const UGA_MeleeAttack* MeleeGA = Cast<UGA_MeleeAttack>(ASC->GetCurrentAbilityInstance()))
		{
			ActionData = MeleeGA->GetAbilityActionData();
			ActionDataPtr = &ActionData;
		}
	}

	if (!ActionDataPtr)
	{
		if (const UAN_MeleeDamage* DamageNotify = FindFirstMeleeDamageNotify(Animation))
		{
			ActionData = DamageNotify->BuildActionData();
			ActionDataPtr = &ActionData;
		}
	}

	VFXBindingComponent->ActivateSlot(SlotName, ActionDataPtr, RemainTime);

	if (bGateMontageSectionWithRemainTime && RemainTime > KINDA_SMALL_NUMBER)
	{
		if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
		{
			if (UAnimMontage* Montage = AnimInstance->GetCurrentActiveMontage())
			{
				FChargeGateRuntime Runtime;
				Runtime.AnimInstance = AnimInstance;
				Runtime.Montage = Montage;
				Runtime.ChargeDuration = RemainTime;
				Runtime.ChargeSectionName = AnimInstance->Montage_GetCurrentSection(Montage);

				if (bLoopCurrentSectionUntilCharged && !Runtime.ChargeSectionName.IsNone() && !ReleaseSectionName.IsNone())
				{
					AnimInstance->Montage_SetNextSection(Runtime.ChargeSectionName, Runtime.ChargeSectionName, Montage);
				}

				FChargeGateRuntime& StoredRuntime =
					ChargeGateRuntimeByMesh.Add(TObjectKey<USkeletalMeshComponent>(MeshComp), Runtime);
				VFXBindingComponent->SetAnnulusPlaneProgress(SlotName, 0.f);

				if (UWorld* World = MeshComp->GetWorld())
				{
					StoredRuntime.StartWorldTime = World->GetTimeSeconds();
					constexpr float ChargeProgressTickInterval = 1.f / 30.f;
					World->GetTimerManager().SetTimer(
						StoredRuntime.ChargeTimerHandle,
						FTimerDelegate::CreateUObject(this, &UANS_MontageVFXBinding::UpdateChargeGate, MeshComp),
						ChargeProgressTickInterval,
						true);
				}
				else
				{
					ReleaseChargeGate(MeshComp, false);
				}
			}
		}
	}
}

void UANS_MontageVFXBinding::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp || SlotName.IsNone())
	{
		return;
	}

	if (bGateMontageSectionWithRemainTime)
	{
		const TObjectKey<USkeletalMeshComponent> MeshKey(MeshComp);
		if (FChargeGateRuntime* Runtime = ChargeGateRuntimeByMesh.Find(MeshKey))
		{
			UAnimInstance* AnimInstance = Runtime->AnimInstance.Get();
			UAnimMontage* Montage = Runtime->Montage.Get();
			const bool bStillInChargeLoop = AnimInstance && Montage
				&& AnimInstance->Montage_IsActive(Montage)
				&& (Runtime->ChargeSectionName.IsNone()
					|| AnimInstance->Montage_GetCurrentSection(Montage) == Runtime->ChargeSectionName);
			if (bStillInChargeLoop && Runtime->ElapsedTime < Runtime->ChargeDuration)
			{
				return;
			}
		}
	}

	ReleaseChargeGate(MeshComp, false);

	const AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
	UMontageVFXBindingComponent* VFXBindingComponent = Character
		? Character->FindComponentByClass<UMontageVFXBindingComponent>()
		: nullptr;
	if (!VFXBindingComponent)
	{
		return;
	}

	VFXBindingComponent->DeactivateSlot(SlotName);
}

void UANS_MontageVFXBinding::UpdateChargeGate(USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp || SlotName.IsNone() || !bGateMontageSectionWithRemainTime || RemainTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	FChargeGateRuntime* Runtime = ChargeGateRuntimeByMesh.Find(TObjectKey<USkeletalMeshComponent>(MeshComp));
	if (!Runtime)
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();
	if (!World)
	{
		ReleaseChargeGate(MeshComp, false);
		return;
	}

	Runtime->ElapsedTime = FMath::Max(World->GetTimeSeconds() - Runtime->StartWorldTime, 0.f);
	const float Progress = FMath::Clamp(Runtime->ElapsedTime / Runtime->ChargeDuration, 0.f, 1.f);

	const AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
	if (UMontageVFXBindingComponent* VFXBindingComponent = Character
		? Character->FindComponentByClass<UMontageVFXBindingComponent>()
		: nullptr)
	{
		VFXBindingComponent->SetAnnulusPlaneProgress(SlotName, Progress);
	}

	if (Progress >= 1.f)
	{
		ReleaseChargeGate(MeshComp, bJumpToReleaseSectionWhenCharged);
	}
}

void UANS_MontageVFXBinding::ReleaseChargeGate(USkeletalMeshComponent* MeshComp, bool bJumpToReleaseSection)
{
	if (!MeshComp)
	{
		return;
	}

	FChargeGateRuntime Runtime;
	const TObjectKey<USkeletalMeshComponent> MeshKey(MeshComp);
	if (!ChargeGateRuntimeByMesh.RemoveAndCopyValue(MeshKey, Runtime))
	{
		return;
	}

	if (UWorld* World = MeshComp->GetWorld())
	{
		World->GetTimerManager().ClearTimer(Runtime.ChargeTimerHandle);
	}

	UAnimInstance* AnimInstance = Runtime.AnimInstance.Get();
	UAnimMontage* Montage = Runtime.Montage.Get();
	if (!AnimInstance || !Montage || !AnimInstance->Montage_IsActive(Montage))
	{
		return;
	}

	if (bLoopCurrentSectionUntilCharged && !Runtime.ChargeSectionName.IsNone() && !ReleaseSectionName.IsNone())
	{
		AnimInstance->Montage_SetNextSection(Runtime.ChargeSectionName, ReleaseSectionName, Montage);
	}

	if (bJumpToReleaseSection && !ReleaseSectionName.IsNone())
	{
		AnimInstance->Montage_JumpToSection(ReleaseSectionName, Montage);
	}
}

FString UANS_MontageVFXBinding::GetNotifyName_Implementation() const
{
	if (!SlotName.IsNone())
	{
		return FString::Printf(TEXT("VFX Binding [%s]"), *SlotName.ToString());
	}
	return TEXT("Montage VFX Binding");
}
