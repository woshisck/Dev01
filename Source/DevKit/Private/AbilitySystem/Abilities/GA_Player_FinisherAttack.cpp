#include "AbilitySystem/Abilities/GA_Player_FinisherAttack.h"

#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"

static const FGameplayTag TAG_Action_Player_FinisherAttack =
	FGameplayTag::RequestGameplayTag(TEXT("Action.Player.FinisherAttack"));

static const FGameplayTag TAG_Action_Finisher_Confirm =
	FGameplayTag::RequestGameplayTag(TEXT("Action.Finisher.Confirm"));

static const FGameplayTag TAG_Ability_Event_Finisher_HitFrame =
	FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Finisher.HitFrame"));

static const FGameplayTag TAG_Buff_Status_Mark_Finisher =
	FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Mark.Finisher"));

static const FGameplayTag TAG_Buff_Status_Dead =
	FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead"));

static const FGameplayTag TAG_Action_Mark_Detonate_Finisher =
	FGameplayTag::RequestGameplayTag(TEXT("Action.Mark.Detonate.Finisher"));

UGA_Player_FinisherAttack::UGA_Player_FinisherAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = TAG_Action_Player_FinisherAttack;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
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

	if (!FinisherMontage)
	{
		UE_LOG(LogTemp, Error, TEXT("[GA_Player_FinisherAttack] FinisherMontage is not configured."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

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
}

void UGA_Player_FinisherAttack::OnMontageEvent(FGameplayTag EventTag, FGameplayEventData EventData)
{
	if (EventTag == TAG_Action_Finisher_Confirm)
	{
		if (!bPlayerConfirmed)
		{
			bPlayerConfirmed = true;
			RestoreTimeDilation();
		}
	}
	else if (EventTag == TAG_Ability_Event_Finisher_HitFrame)
	{
		DetonateMarks(bPlayerConfirmed);
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
		if (!Target->GetASC()->HasMatchingGameplayTag(TAG_Buff_Status_Mark_Finisher))
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

void UGA_Player_FinisherAttack::RestoreTimeDilation()
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
}

void UGA_Player_FinisherAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	RestoreTimeDilation();

	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
