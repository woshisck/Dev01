#include "AbilitySystem/Abilities/GA_Player_FinisherAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AN_MeleeDamage.h"
#include "Animation/ANS_FinisherTimeDilation.h"
#include "Character/YogCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/MontageAttackDataAsset.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "UI/YogHUD.h"
#include "Visual/TimeDilationVisualSubsystem.h"

static const FGameplayTag TAG_Action_Player_FinisherAttack =
	FGameplayTag::RequestGameplayTag(TEXT("Action.Player.FinisherAttack"));

static const FGameplayTag TAG_Action_Finisher_Confirm =
	FGameplayTag::RequestGameplayTag(TEXT("Action.Finisher.Confirm"));

static const FGameplayTag TAG_Ability_Event_Finisher_HitFrame =
	FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Finisher.HitFrame"));

static const FGameplayTag TAG_PlayerFinisherAttack_Buff_Status_Mark_Finisher =
	FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Mark.Finisher"));

static const FGameplayTag TAG_Buff_Status_Dead =
	FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"));

static const FGameplayTag TAG_Action_Mark_Detonate_Finisher =
	FGameplayTag::RequestGameplayTag(TEXT("Action.Mark.Detonate.Finisher"));

static const FGameplayTag TAG_Buff_Status_FinisherQTEOpen =
	FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherQTEOpen"));

UGA_Player_FinisherAttack::UGA_Player_FinisherAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = TAG_Action_Player_FinisherAttack;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	FallbackFinisherActionData.ActDamage = 60.f;
	FallbackFinisherActionData.ActRange = 480.f;
	FallbackFinisherActionData.ActResilience = 40.f;

	FYogHitboxType DefaultHitbox;
	DefaultHitbox.hitboxType = EHitBoxType::Annulus;
	DefaultHitbox.AnnulusHitbox.inner_radius = 0.f;
	DefaultHitbox.AnnulusHitbox.degree = 115.f;
	DefaultHitbox.AnnulusHitbox.bAutoOffset = true;
	FallbackFinisherActionData.hitboxTypes.Add(DefaultHitbox);
}

void UGA_Player_FinisherAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bPlayerConfirmed = false;
	bTimeDilationRestored = false;
	bDetonated = false;
	bReceivedHitFrame = false;
	bFallbackQTEOpened = false;
	bPreFinisherAuraActive = false;
	bTimeDilationVisualActive = false;

	if (!FinisherMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("[GA_Player_FinisherAttack] FinisherMontage is not configured."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartPreFinisherAura();

	FGameplayTagContainer EventTags;
	EventTags.AddTag(TAG_Action_Finisher_Confirm);
	EventTags.AddTag(TAG_Ability_Event_Finisher_HitFrame);

	MontageTask = UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		NAME_None,
		FinisherMontage,
		EventTags,
		1.f,
		NAME_None,
		true);

	if (MontageTask)
	{
		MontageTask->EventReceived.AddDynamic(this, &UGA_Player_FinisherAttack::OnMontageEvent);
		MontageTask->OnCompleted.AddDynamic(this, &UGA_Player_FinisherAttack::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_Player_FinisherAttack::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_Player_FinisherAttack::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_Player_FinisherAttack::OnMontageInterrupted);
		MontageTask->ReadyForActivation();
	}

	if (bEnableFallbackTimeDilationWindow && !MontageHasFinisherTimeDilationNotify())
	{
		ScheduleTicker(FallbackQTEStartTickerHandle, FallbackQTEWindowStartTime, [this]()
		{
			OpenFallbackQTEWindow();
		});
		UE_LOG(LogTemp, Warning,
			TEXT("[GA_Player_FinisherAttack] Finisher montage has no ANS_FinisherTimeDilation. Fallback QTE window scheduled at %.2fs for %.2fs."),
			FallbackQTEWindowStartTime,
			FallbackQTEWindowDuration);
	}

	if (bEnableFallbackHitFrame && !MontageHasFinisherHitFrameNotify())
	{
		ScheduleTicker(FallbackHitFrameTickerHandle, FallbackHitFrameTime, [this]()
		{
			TriggerFallbackHitFrame();
		});
		UE_LOG(LogTemp, Warning,
			TEXT("[GA_Player_FinisherAttack] Finisher montage has no AN_MeleeDamage HitFrame. Fallback hit frame scheduled at %.2fs."),
			FallbackHitFrameTime);
	}
}

void UGA_Player_FinisherAttack::OnMontageEvent(FGameplayTag EventTag, FGameplayEventData EventData)
{
	if (EventTag == TAG_Action_Finisher_Confirm)
	{
		if (!bPlayerConfirmed)
		{
			UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
			if (!ASC || !ASC->HasMatchingGameplayTag(TAG_Buff_Status_FinisherQTEOpen))
			{
				return;
			}

			bPlayerConfirmed = true;
			bFallbackQTEOpened = false;
			ClearTicker(FallbackQTEEndTickerHandle);
			ASC->SetLooseGameplayTagCount(TAG_Buff_Status_FinisherQTEOpen, 0);
			RestoreTimeDilation(true);

			if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo()))
			{
				if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
				{
					if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
					{
						HUD->MarkFinisherQTEConfirmed();
						HUD->HideFinisherQTEPrompt();
					}
				}
			}
		}
	}
	else if (EventTag == TAG_Ability_Event_Finisher_HitFrame)
	{
		HandleFinisherHitFrame(EventData);
	}
}

void UGA_Player_FinisherAttack::OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Player_FinisherAttack::OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Player_FinisherAttack::DetonateMarks(bool bConfirmed)
{
	if (bDetonated)
	{
		return;
	}
	bDetonated = true;

	UWorld* World = GetWorld();
	UAbilitySystemComponent* PlayerASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!World || !PlayerASC)
	{
		return;
	}

	AActor* PlayerActor = GetAvatarActorFromActorInfo();
	const float ConfirmMultiplier = bConfirmed ? ConfirmedDamageMultiplier : 1.f;

	TArray<AActor*> AllCharacters;
	UGameplayStatics::GetAllActorsOfClass(World, AYogCharacterBase::StaticClass(), AllCharacters);
	for (AActor* Actor : AllCharacters)
	{
		AYogCharacterBase* Target = Cast<AYogCharacterBase>(Actor);
		if (!Target || !Target->GetASC())
		{
			continue;
		}
		if (Target->GetASC()->HasMatchingGameplayTag(TAG_Buff_Status_Dead))
		{
			continue;
		}
		if (!Target->GetASC()->HasMatchingGameplayTag(TAG_PlayerFinisherAttack_Buff_Status_Mark_Finisher))
		{
			continue;
		}

		FGameplayEventData DetEvent;
		DetEvent.Instigator = PlayerActor;
		DetEvent.Target = Target;
		DetEvent.EventMagnitude = ConfirmMultiplier;
		PlayerASC->HandleGameplayEvent(TAG_Action_Mark_Detonate_Finisher, &DetEvent);
	}
}

void UGA_Player_FinisherAttack::ApplyFinisherHitbox(const FGameplayEventData& EventData)
{
	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());
	if (!Owner || !Owner->DefaultMeleeTargetType || !Owner->DefaultMeleeDamageEffect)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GA_Player_FinisherAttack] Skip direct finisher hitbox. Owner=%s TargetType=%s DamageGE=%s"),
			*GetNameSafe(Owner),
			Owner ? *GetNameSafe(Owner->DefaultMeleeTargetType.Get()) : TEXT("None"),
			Owner ? *GetNameSafe(Owner->DefaultMeleeDamageEffect.Get()) : TEXT("None"));
		return;
	}

	FYogGameplayEffectContainerSpec ContainerSpec = MakeEffectContainerSpec(TAG_Ability_Event_Finisher_HitFrame, EventData, -1);
	const TArray<FActiveGameplayEffectHandle> Handles = ApplyEffectContainerSpec(ContainerSpec);
	if (Handles.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_Player_FinisherAttack] Finisher hitbox found no valid targets."));
		return;
	}

	Owner->bComboHitConnected = true;

	TArray<AActor*> HitActors;
	for (const TSharedPtr<FGameplayAbilityTargetData>& Data : ContainerSpec.TargetData.Data)
	{
		if (!Data.IsValid())
		{
			continue;
		}

		for (TWeakObjectPtr<AActor> WeakActor : Data->GetActors())
		{
			if (AActor* Actor = WeakActor.Get())
			{
				HitActors.AddUnique(Actor);
			}
		}
	}

	static const FGameplayTag HitTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit"));
	for (AActor* HitActor : HitActors)
	{
		FGameplayEventData Payload;
		Payload.Instigator = Owner;
		Payload.Target = HitActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, HitTag, Payload);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[GA_Player_FinisherAttack] Applied finisher hitbox. Hits=%d DamageHandles=%d Confirmed=%d"),
		HitActors.Num(),
		Handles.Num(),
		bPlayerConfirmed ? 1 : 0);
}

void UGA_Player_FinisherAttack::HandleFinisherHitFrame(const FGameplayEventData& EventData)
{
	if (bReceivedHitFrame)
	{
		return;
	}

	bReceivedHitFrame = true;
	ClearTicker(FallbackHitFrameTickerHandle);

	ApplyFinisherHitbox(EventData);
	DetonateMarks(bPlayerConfirmed);
}

void UGA_Player_FinisherAttack::RestoreTimeDilation(bool bEndExternalVisual)
{
	if (bTimeDilationRestored)
	{
		return;
	}
	bTimeDilationRestored = true;

	if (AWorldSettings* WorldSettings = GetWorld() ? GetWorld()->GetWorldSettings() : nullptr)
	{
		WorldSettings->SetTimeDilation(1.f);
	}

	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		Avatar->CustomTimeDilation = 1.f;
	}

	if (bTimeDilationVisualActive)
	{
		UTimeDilationVisualSubsystem::EndTimeDilationVisual(GetAvatarActorFromActorInfo());
		bTimeDilationVisualActive = false;
	}
	else if (bEndExternalVisual)
	{
		UTimeDilationVisualSubsystem::EndTimeDilationVisual(GetAvatarActorFromActorInfo());
	}
}

void UGA_Player_FinisherAttack::OpenFallbackQTEWindow()
{
	if (bPlayerConfirmed || bDetonated)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC && ASC->HasMatchingGameplayTag(TAG_Buff_Status_FinisherQTEOpen))
	{
		return;
	}

	UWorld* World = GetWorld();
	AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo());
	if (!World || !Character)
	{
		return;
	}

	const float SafeDilation = FMath::Clamp(FallbackQTESlowDilation, 0.001f, 1.f);
	if (AWorldSettings* WorldSettings = World->GetWorldSettings())
	{
		WorldSettings->SetTimeDilation(SafeDilation);
	}
	UTimeDilationVisualSubsystem::BeginTimeDilationVisual(Character);
	bTimeDilationVisualActive = true;
	Character->CustomTimeDilation = 1.f / SafeDilation;
	bTimeDilationRestored = false;
	bFallbackQTEOpened = true;

	if (ASC)
	{
		ASC->SetLooseGameplayTagCount(TAG_Buff_Status_FinisherQTEOpen, 1);
	}

	if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			HUD->ShowFinisherQTEPrompt(FallbackQTEWindowDuration);
		}
	}

	ScheduleTicker(FallbackQTEEndTickerHandle, FallbackQTEWindowDuration, [this]()
	{
		CloseFallbackQTEWindow();
	});
}

void UGA_Player_FinisherAttack::CloseFallbackQTEWindow()
{
	if (!bFallbackQTEOpened)
	{
		return;
	}

	bFallbackQTEOpened = false;
	RestoreTimeDilation();

	if (UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		ASC->SetLooseGameplayTagCount(TAG_Buff_Status_FinisherQTEOpen, 0);
	}

	if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			{
				HUD->HideFinisherQTEPrompt();
			}
		}
	}
}

void UGA_Player_FinisherAttack::TriggerFallbackHitFrame()
{
	if (bReceivedHitFrame)
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.Instigator = GetAvatarActorFromActorInfo();
	EventData.OptionalObject = this;
	HandleFinisherHitFrame(EventData);
}

void UGA_Player_FinisherAttack::ScheduleTicker(FTSTicker::FDelegateHandle& Handle, float DelaySeconds, TFunction<void()> Callback)
{
	ClearTicker(Handle);

	if (DelaySeconds <= 0.f)
	{
		Callback();
		return;
	}

	TWeakObjectPtr<UGA_Player_FinisherAttack> WeakThis(this);
	Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[WeakThis, Callback = MoveTemp(Callback)](float)
		{
			if (WeakThis.IsValid())
			{
				Callback();
			}
			return false;
		}),
		DelaySeconds);
}

void UGA_Player_FinisherAttack::ClearTicker(FTSTicker::FDelegateHandle& Handle)
{
	if (Handle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(Handle);
		Handle.Reset();
	}
}

void UGA_Player_FinisherAttack::ClearFallbackTickers()
{
	ClearTicker(FallbackQTEStartTickerHandle);
	ClearTicker(FallbackQTEEndTickerHandle);
	ClearTicker(FallbackHitFrameTickerHandle);
	ClearTicker(PreFinisherAuraTickerHandle);
}

bool UGA_Player_FinisherAttack::MontageHasFinisherTimeDilationNotify() const
{
	if (!FinisherMontage)
	{
		return false;
	}

	for (const FAnimNotifyEvent& NotifyEvent : FinisherMontage->Notifies)
	{
		if (NotifyEvent.NotifyStateClass && NotifyEvent.NotifyStateClass->IsA(UANS_FinisherTimeDilation::StaticClass()))
		{
			return true;
		}
	}

	return false;
}

bool UGA_Player_FinisherAttack::MontageHasFinisherHitFrameNotify() const
{
	if (!FinisherMontage)
	{
		return false;
	}

	for (const FAnimNotifyEvent& NotifyEvent : FinisherMontage->Notifies)
	{
		const UAN_MeleeDamage* DamageNotify = Cast<UAN_MeleeDamage>(NotifyEvent.Notify);
		if (!DamageNotify)
		{
			continue;
		}

		const FGameplayTag NotifyEventTag = DamageNotify->AttackDataOverride && DamageNotify->AttackDataOverride->EventTag.IsValid()
			? DamageNotify->AttackDataOverride->EventTag
			: DamageNotify->EventTag;
		if (NotifyEventTag == TAG_Ability_Event_Finisher_HitFrame)
		{
			return true;
		}
	}

	return false;
}

FActionData UGA_Player_FinisherAttack::GetAbilityActionData_Implementation() const
{
	return FallbackFinisherActionData;
}

void UGA_Player_FinisherAttack::StartPreFinisherAura()
{
	if (bPreFinisherAuraActive)
	{
		return;
	}

	AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	bPreFinisherAuraActive = true;
	Character->StartFinisherAuraFlash();

	if (PreFinisherAuraNiagara)
	{
		USceneComponent* AttachComponent = Character->GetMesh() ? Cast<USceneComponent>(Character->GetMesh()) : Character->GetRootComponent();
		if (AttachComponent)
		{
			PreFinisherAuraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				PreFinisherAuraNiagara,
				AttachComponent,
				PreFinisherAuraAttachSocketName,
				PreFinisherAuraLocationOffset,
				PreFinisherAuraRotationOffset,
				EAttachLocation::KeepRelativeOffset,
				true);

			if (PreFinisherAuraComponent)
			{
				PreFinisherAuraComponent->SetRelativeScale3D(PreFinisherAuraScale);
			}
		}
	}

	BP_OnPreFinisherAuraStarted(PreFinisherAuraComponent);

	if (PreFinisherAuraDuration > 0.f)
	{
		ScheduleTicker(PreFinisherAuraTickerHandle, PreFinisherAuraDuration, [this]()
		{
			StopPreFinisherAura();
		});
	}
}

void UGA_Player_FinisherAttack::StopPreFinisherAura()
{
	if (!bPreFinisherAuraActive && !PreFinisherAuraComponent)
	{
		return;
	}

	ClearTicker(PreFinisherAuraTickerHandle);
	bPreFinisherAuraActive = false;

	if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		Character->StopFinisherAuraFlash();
	}

	if (PreFinisherAuraComponent)
	{
		PreFinisherAuraComponent->Deactivate();
		if (bDestroyPreFinisherAuraOnAbilityEnd)
		{
			PreFinisherAuraComponent->DestroyComponent();
		}
		PreFinisherAuraComponent = nullptr;
	}

	BP_OnPreFinisherAuraEnded();
}

void UGA_Player_FinisherAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ClearFallbackTickers();
	RestoreTimeDilation();
	StopPreFinisherAura();

	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		ASC->SetLooseGameplayTagCount(TAG_Buff_Status_FinisherQTEOpen, 0);
	}

	if (AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			{
				HUD->HideFinisherQTEPrompt();
			}
		}
	}

	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
