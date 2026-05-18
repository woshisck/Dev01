#include "Component/ComboRuntimeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_PlayMontage.h"
#include "Character/PlayerCharacterBase.h"
#include "Animation/AnimMontage.h"
#include "Component/CombatDeckComponent.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageConfigDA.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Particles/ParticleSystem.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

namespace
{
	FGameplayTag CardActionToInputTag(ECardRequiredAction Action)
	{
		// ECardRequiredAction::Any → invalid tag (matches anything per the plugin's convention).
		static const FName LightName(TEXT("Combo.Input.Light"));
		static const FName HeavyName(TEXT("Combo.Input.Heavy"));
		switch (Action)
		{
			case ECardRequiredAction::Light: return FGameplayTag::RequestGameplayTag(LightName, false);
			case ECardRequiredAction::Heavy: return FGameplayTag::RequestGameplayTag(HeavyName, false);
			case ECardRequiredAction::Any:   return FGameplayTag();
			default:                          return FGameplayTag();
		}
	}

	bool IsLegacyComboProgressTag(const FGameplayTag& Tag)
	{
		const FString TagName = Tag.ToString();
		return TagName.StartsWith(TEXT("PlayerState.AbilityCast.LightAtk.Combo"))
			|| TagName.StartsWith(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo"));
	}

	void AddLegacyComboProgressTags(FGameplayTagContainer& OutTags)
	{
		static const FName KnownComboTagNames[] = {
			TEXT("PlayerState.AbilityCast.LightAtk.Combo1"),
			TEXT("PlayerState.AbilityCast.LightAtk.Combo2"),
			TEXT("PlayerState.AbilityCast.LightAtk.Combo3"),
			TEXT("PlayerState.AbilityCast.LightAtk.Combo4"),
			TEXT("PlayerState.AbilityCast.HeavyAtk.Combo1"),
			TEXT("PlayerState.AbilityCast.HeavyAtk.Combo2"),
			TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3"),
			TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4"),
		};

		for (const FName& TagName : KnownComboTagNames)
		{
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, false);
			if (Tag.IsValid())
			{
				OutTags.AddTag(Tag);
			}
		}
	}

	void ClearComboWindowAndProgressLooseTags(UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return;
		}

		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"), false);
		if (CanComboTag.IsValid())
		{
			ASC->SetLooseGameplayTagCount(CanComboTag, 0);
		}

		FGameplayTagContainer ProgressTags;
		AddLegacyComboProgressTags(ProgressTags);
		for (const FGameplayTag& Tag : ProgressTags)
		{
			if (ASC->GetTagCount(Tag) > 0)
			{
				ASC->SetLooseGameplayTagCount(Tag, 0);
			}
		}
	}
}

UComboRuntimeComponent::UComboRuntimeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UComboRuntimeComponent::LoadComboConfig(UWeaponComboConfigDA* InComboConfig)
{
	ComboConfig = InComboConfig;
	ComboGraph = nullptr;
	ResetCombo();

	if (!ComboConfig)
	{
		return;
	}

	TArray<FText> Warnings;
	ComboConfig->ValidateConfig(Warnings);
	for (const FText& Warning : Warnings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponComboConfig] %s: %s"), *GetNameSafe(ComboConfig), *Warning.ToString());
	}
}

void UComboRuntimeComponent::LoadComboGraph(UGameplayAbilityComboGraph* InComboGraph)
{
	ComboGraph = InComboGraph;
	ComboConfig = nullptr;
	ResetCombo();

	if (!ComboGraph)
	{
		return;
	}

	TArray<FText> Warnings;
	ComboGraph->ValidateComboGraph(Warnings);
	for (const FText& Warning : Warnings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameplayAbilityComboGraph] %s: %s"), *GetNameSafe(ComboGraph), *Warning.ToString());
	}
}

bool UComboRuntimeComponent::TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner)
{
	if ((!ComboConfig && !ComboGraph) || !PlayerOwner)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = PlayerOwner->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	const bool bUseDashSavedNode = !SavedDashNodeId.IsNone();
	const FName StartNodeId = bUseDashSavedNode ? SavedDashNodeId : CurrentNodeId;

	// Combo window gate: while a combo is already in progress, only allow advancing
	// to the next node when the active node's window is open (CanCombo tag is set by
	// either ANS_ComboWindow or the node-driven timers in GA_PlayMontage).
	// Fresh combo starts (CurrentNodeId is None) and dash-save re-entries skip the gate.
	if (!CurrentNodeId.IsNone() && !bUseDashSavedNode)
	{
		static const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		if (ASC->GetTagCount(CanComboTag) <= 0)
		{
			return false;
		}
	}

	FWeaponComboNodeConfig GraphNodeConfig;
	const FWeaponComboNodeConfig* NextNode = nullptr;
	bool bFoundChildNode = false;

	if (ComboGraph)
	{
		const FGameplayTag InputTag = CardActionToInputTag(InputAction);
		FGameplayTagContainer OwnedTags;
		ASC->GetOwnedGameplayTags(OwnedTags);
		const UGameplayAbilityComboGraphNode* NextGraphNode = ComboGraph->FindChildComboNode(StartNodeId, InputTag, &OwnedTags);
		bFoundChildNode = NextGraphNode != nullptr;
		if (!NextGraphNode)
		{
			NextGraphNode = ComboGraph->FindRootComboNode(InputTag);
		}
		if (NextGraphNode)
		{
			GraphNodeConfig = FWeaponComboNodeConfig::FromComboGraphNode(NextGraphNode, InputAction);
			NextNode = &GraphNodeConfig;
		}
	}
	else if (ComboConfig)
	{
		NextNode = ComboConfig->FindChildNode(StartNodeId, InputAction);
		bFoundChildNode = NextNode != nullptr;
		if (!NextNode)
		{
			NextNode = ComboConfig->FindRootNode(InputAction);
		}
	}

	if (!NextNode || (!NextNode->Montage && !NextNode->MontageConfig))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] No combo node for input=%s current=%s graph=%s config=%s"),
			*StaticEnum<ECardRequiredAction>()->GetNameStringByValue(static_cast<int64>(InputAction)),
			*CurrentNodeId.ToString(),
			*GetNameSafe(ComboGraph),
			*GetNameSafe(ComboConfig));
		if (!CurrentNodeId.IsNone() && PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
		}
		bComboContinued = false;
		bExitedComboState = !CurrentNodeId.IsNone();
		CurrentNodeId = NAME_None;
		ComboIndex = 0;
		ComboTags.Reset();
		return false;
	}

	ActiveNode = *NextNode;
	bActiveNodeValid = true;
	bActivationFromDashSave = bUseDashSavedNode;
	bComboContinued = bFoundChildNode || bUseDashSavedNode;
	bExitedComboState = !CurrentNodeId.IsNone() && !bFoundChildNode && !bUseDashSavedNode;
	ActiveAttackGuid = FGuid::NewGuid();

	if (bExitedComboState)
	{
		FGameplayTagContainer TagsToCancel;
		AddLegacyComboProgressTags(TagsToCancel);
		ASC->CancelAbilities(&TagsToCancel);
		ClearComboWindowAndProgressLooseTags(ASC);
	}

	const bool bActivated = ASC->TryActivateAbilityByClass(UGA_PlayMontage::StaticClass());

	if (!bActivated)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ComboRuntime] Failed to activate node=%s input=%s current=%s montage=%s montageConfig=%s"),
			*NextNode->NodeId.ToString(),
			*StaticEnum<ECardRequiredAction>()->GetNameStringByValue(static_cast<int64>(InputAction)),
			*CurrentNodeId.ToString(),
			*GetNameSafe(NextNode->Montage.Get()),
			*GetNameSafe(NextNode->MontageConfig.Get()));
		bActiveNodeValid = false;
		ActiveNode = FWeaponComboNodeConfig();
		ActiveAttackGuid.Invalidate();
		bActivationFromDashSave = false;
		bComboContinued = false;
		return false;
	}

	ComboIndex = bFoundChildNode || bUseDashSavedNode
		? FMath::Max(1, ComboIndex + 1)
		: 1;
	ComboTags.Reset();

	CurrentNodeId = NextNode->NodeId;
	SavedDashNodeId = NAME_None;
	if (bUseDashSavedNode && PlayerOwner->CombatDeckComponent)
	{
		PlayerOwner->CombatDeckComponent->RestorePendingLinkContextFromDash();
	}
	return true;
}

void UComboRuntimeComponent::ResetCombo()
{
	CurrentNodeId = NAME_None;
	SavedDashNodeId = NAME_None;
	ActiveNode = FWeaponComboNodeConfig();
	ActiveAttackGuid.Invalidate();
	ComboIndex = 0;
	ComboTags.Reset();
	bActiveNodeValid = false;
	bActivationFromDashSave = false;
	bComboContinued = false;
	bExitedComboState = true;
	if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetOwner()))
	{
		if (PlayerOwner->CombatDeckComponent)
		{
			PlayerOwner->CombatDeckComponent->NotifyComboStateExited();
		}
	}
}

void UComboRuntimeComponent::SaveCurrentNodeForDash()
{
	if (bActiveNodeValid && ActiveNode.bAllowDashSave)
	{
		SavedDashNodeId = ActiveNode.NodeId;
	}
}

const FWeaponComboNodeConfig* UComboRuntimeComponent::GetActiveNode() const
{
	return bActiveNodeValid ? &ActiveNode : nullptr;
}

bool UComboRuntimeComponent::ConsumeActivationFromDashSave()
{
	const bool bResult = bActivationFromDashSave;
	bActivationFromDashSave = false;
	return bResult;
}

FCombatDeckActionContext UComboRuntimeComponent::BuildAttackContext(ECombatCardTriggerTiming TriggerTiming, APlayerCharacterBase* PlayerOwner) const
{
	FCombatDeckActionContext Context;
	if (!bActiveNodeValid)
	{
		return Context;
	}

	Context.ActionType = ActiveNode.InputAction;
	Context.ComboIndex = ComboIndex;
	Context.ComboNodeId = ActiveNode.NodeId;
	Context.ComboTags = ComboTags;
	Context.AbilityTag = ActiveNode.AbilityTag;
	Context.WeaponDef = PlayerOwner ? PlayerOwner->EquippedWeaponDef : nullptr;
	Context.bIsComboFinisher = ActiveNode.bIsComboFinisher;
	Context.bComboContinued = bComboContinued;
	Context.bExitedComboState = bExitedComboState;
	Context.bFromDashSave = bActivationFromDashSave;
	Context.TriggerTiming = TriggerTiming;
	Context.AttackInstanceGuid = ActiveAttackGuid;
	return Context;
}

void UComboRuntimeComponent::NotifyMontageStarted()
{
	if (!bActiveNodeValid)
	{
		return;
	}
	PlayFxBinding(ActiveNode.OnMontageStartFx);
}

void UComboRuntimeComponent::NotifyHitLanded()
{
	if (!bActiveNodeValid)
	{
		return;
	}
	PlayFxBinding(ActiveNode.OnHitSuccessFx);
	ApplyHitDilation(ActiveNode.HitSuccessDilation);
}

void UComboRuntimeComponent::PlayFxBinding(const FComboNodeFxBinding& Binding)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (Binding.Sound)
	{
		UGameplayStatics::SpawnSoundAtLocation(OwnerActor, Binding.Sound, OwnerActor->GetActorLocation());
	}

	if (UFXSystemAsset* FxAsset = Binding.ParticleSystem)
	{
		USceneComponent* AttachTo = OwnerActor->GetRootComponent();
		const FName Socket = Binding.AttachSocket;

		if (UNiagaraSystem* NiagaraSys = Cast<UNiagaraSystem>(FxAsset))
		{
			if (AttachTo)
			{
				UNiagaraFunctionLibrary::SpawnSystemAttached(
					NiagaraSys, AttachTo, Socket,
					FVector::ZeroVector, FRotator::ZeroRotator,
					EAttachLocation::SnapToTarget, true);
			}
			else
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(
					OwnerActor, NiagaraSys, OwnerActor->GetActorLocation());
			}
		}
		else if (UParticleSystem* LegacyParticle = Cast<UParticleSystem>(FxAsset))
		{
			if (AttachTo)
			{
				UGameplayStatics::SpawnEmitterAttached(
					LegacyParticle, AttachTo, Socket,
					FVector::ZeroVector, FRotator::ZeroRotator,
					EAttachLocation::SnapToTarget, true);
			}
			else
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					OwnerActor, LegacyParticle, OwnerActor->GetActorLocation());
			}
		}
	}
}

void UComboRuntimeComponent::ApplyHitDilation(const FComboHitDilationSettings& Settings)
{
	if (Settings.Scope == EComboHitDilationScope::None || Settings.DurationSeconds <= 0.f)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	// A new hit-dilation overrides an in-flight one. Restore any prior scope first so we don't leak state.
	if (ActiveDilationScope != EComboHitDilationScope::None)
	{
		World->GetTimerManager().ClearTimer(DilationRestoreHandle);
		RestoreTimeDilation();
	}

	const float Factor = FMath::Clamp(Settings.DilationFactor, 0.01f, 1.0f);
	ActiveDilationScope = Settings.Scope;

	float TimerDuration = Settings.DurationSeconds;
	if (Settings.Scope == EComboHitDilationScope::Global)
	{
		UGameplayStatics::SetGlobalTimeDilation(World, Factor);
		// World timers count game time, which now advances at Factor × real time.
		// Multiply so the timer fires after DurationSeconds *real* seconds.
		TimerDuration *= Factor;
	}
	else // Self
	{
		OwnerActor->CustomTimeDilation = Factor;
	}

	World->GetTimerManager().SetTimer(
		DilationRestoreHandle, this, &UComboRuntimeComponent::RestoreTimeDilation,
		FMath::Max(TimerDuration, 0.01f), false);
}

void UComboRuntimeComponent::RestoreTimeDilation()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;

	if (ActiveDilationScope == EComboHitDilationScope::Global && World)
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.f);
	}
	else if (ActiveDilationScope == EComboHitDilationScope::Self && OwnerActor)
	{
		OwnerActor->CustomTimeDilation = 1.f;
	}

	ActiveDilationScope = EComboHitDilationScope::None;
}
