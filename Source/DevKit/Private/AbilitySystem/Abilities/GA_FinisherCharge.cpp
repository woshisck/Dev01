#include "AbilitySystem/Abilities/GA_FinisherCharge.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/YogCharacterBase.h"
#include "Data/LevelInfoPopupDA.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/YogHUD.h"

static const FGameplayTag TAG_Action_FinisherCharge_Activate =
	FGameplayTag::RequestGameplayTag(TEXT("Action.FinisherCharge.Activate"));

static const FGameplayTag TAG_Action_FinisherCharge_ChargeConsumed =
	FGameplayTag::RequestGameplayTag(TEXT("Action.FinisherCharge.ChargeConsumed"));

static const FGameplayTag TAG_Buff_Status_FinisherCharge =
	FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherCharge"));

static const FGameplayTag TAG_FinisherCharge_Buff_Status_Mark_Finisher =
	FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Mark.Finisher"));

static const FGameplayTag TAG_Buff_Status_FinisherWindowOpen =
	FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherWindowOpen"));

static const FGameplayTag TAG_Ability_Event_Attack_Hit =
	FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit"));

static const FGameplayTag TAG_Buff_Status_FinisherExecuting =
	FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherExecuting"));

UGA_FinisherCharge::UGA_FinisherCharge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = TAG_Action_FinisherCharge_Activate;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// 让 GA_PlayerDash::PreActivate 的豁免检查能看到这个 GA 的持续 buff 标签，
	// 从而在冲刺时不取消此 GA（Buff.Status.* 被 DashCancelProtectedTags 保护）。
	// GAS 会在技能激活时自动把此 tag 加到 ASC，EndAbility 时自动移除。
	ActivationOwnedTags.AddTag(TAG_Buff_Status_FinisherWindowOpen);
	ComboHintTitle = FText::FromString(TEXT("终结技准备就绪"));
	ComboHintBody = FText::FromString(TEXT("在强化时间内打出 H -> H -> H，最后一击后会自动触发终结技。"));
}

void UGA_FinisherCharge::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	RemainingCharges = 0;
	ChargeGEHandle = FActiveGameplayEffectHandle();

	if (FinisherChargeGEClass)
	{
		FGameplayEffectQuery Query;
		Query.EffectDefinition = FinisherChargeGEClass;
		TArray<FActiveGameplayEffectHandle> Handles = ASC->GetActiveEffects(Query);
		if (Handles.Num() > 0)
		{
			ChargeGEHandle = Handles[0];
			if (const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ChargeGEHandle))
			{
				RemainingCharges = ActiveGE->Spec.GetStackCount();
			}
		}
	}

	if (RemainingCharges <= 0)
	{
		RemainingCharges = MaxCharges;
	}

	ASC->AddLooseGameplayTag(TAG_Buff_Status_FinisherWindowOpen);
	ShowComboHint();

	TagChangedHandle = ASC->RegisterGameplayTagEvent(
		TAG_Buff_Status_FinisherCharge,
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UGA_FinisherCharge::OnFinisherChargeTagChanged);

	WaitHitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		TAG_Ability_Event_Attack_Hit,
		nullptr,
		false,
		true);
	if (WaitHitTask)
	{
		WaitHitTask->EventReceived.AddDynamic(this, &UGA_FinisherCharge::OnAttackHit);
		WaitHitTask->ReadyForActivation();
	}
}

void UGA_FinisherCharge::OnAttackHit(FGameplayEventData EventData)
{
	if (RemainingCharges <= 0)
	{
		return;
	}

	AActor* HitTarget = const_cast<AActor*>(EventData.Target.Get());
	if (!HitTarget)
	{
		return;
	}

	UAbilitySystemComponent* PlayerASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!PlayerASC)
	{
		return;
	}

	FGameplayEventData ChargeConsumedPayload;
	ChargeConsumedPayload.Instigator = GetAvatarActorFromActorInfo();
	ChargeConsumedPayload.Target = HitTarget;
	PlayerASC->HandleGameplayEvent(TAG_Action_FinisherCharge_ChargeConsumed, &ChargeConsumedPayload);

	--RemainingCharges;

	if (ChargeGEHandle.IsValid())
	{
		PlayerASC->RemoveActiveGameplayEffect(ChargeGEHandle, 1);
	}
	else if (RemainingCharges <= 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_FinisherCharge::OnFinisherChargeTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		return;
	}

	if (WaitHitTask)
	{
		WaitHitTask->EndTask();
		WaitHitTask = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		DeferredEndHandle = World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UGA_FinisherCharge::EndAbilityDeferred));
	}
}

void UGA_FinisherCharge::EndAbilityDeferred()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_FinisherCharge::ClearAllMarks()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC && ASC->HasMatchingGameplayTag(TAG_Buff_Status_FinisherExecuting))
	{
		return;
	}

	TArray<AActor*> AllCharacters;
	UGameplayStatics::GetAllActorsOfClass(World, AYogCharacterBase::StaticClass(), AllCharacters);
	for (AActor* Actor : AllCharacters)
	{
		AYogCharacterBase* Character = Cast<AYogCharacterBase>(Actor);
		if (!Character || !Character->GetASC())
		{
			continue;
		}

		if (Character->GetASC()->HasMatchingGameplayTag(TAG_FinisherCharge_Buff_Status_Mark_Finisher))
		{
			Character->GetASC()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TAG_FinisherCharge_Buff_Status_Mark_Finisher));
		}
	}
}

void UGA_FinisherCharge::ShowComboHint()
{
	if (!bShowComboHintOnActivate)
	{
		return;
	}

	AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC)
	{
		return;
	}

	AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD());
	if (!HUD)
	{
		return;
	}

	if (ComboHintPopup)
	{
		HUD->ShowInfoPopup(ComboHintPopup);
		return;
	}

	ULevelInfoPopupDA* TransientPopup = NewObject<ULevelInfoPopupDA>(this);
	TransientPopup->Title = ComboHintTitle;
	TransientPopup->Body = ComboHintBody;
	TransientPopup->HUDSummaryText = ComboHintBody;
	TransientPopup->DisplayDuration = ComboHintDuration;
	HUD->ShowInfoPopup(TransientPopup);
}

void UGA_FinisherCharge::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (DeferredEndHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DeferredEndHandle);
		}
		DeferredEndHandle.Invalidate();
	}

	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		if (TagChangedHandle.IsValid())
		{
			ASC->RegisterGameplayTagEvent(
				TAG_Buff_Status_FinisherCharge,
				EGameplayTagEventType::NewOrRemoved).Remove(TagChangedHandle);
			TagChangedHandle.Reset();
		}

		ASC->RemoveLooseGameplayTag(TAG_Buff_Status_FinisherWindowOpen);

		if (ASC->HasMatchingGameplayTag(TAG_Buff_Status_FinisherCharge))
		{
			ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TAG_Buff_Status_FinisherCharge));
		}
	}

	if (WaitHitTask)
	{
		WaitHitTask->EndTask();
		WaitHitTask = nullptr;
	}

	ClearAllMarks();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
