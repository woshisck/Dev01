// Fill out your copyright notice in the Description page of Project Settings.



#include "DamageAttributeSet.h"
#include "PlayerAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include <DevKit/Character/YogCharacterBase.h>
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"




UDamageAttributeSet::UDamageAttributeSet()
{	
	InitDamagePhysical(0);
	InitDamageMagic(0);
	InitDamagePure(0);
}

void UDamageAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();
	const FGameplayTagContainer& SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();

	// Compute the delta between old and new, if it is available
	float DeltaValue = 0;
	if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	{
		// If this was additive, store the raw delta value to be passed along later
		DeltaValue = Data.EvaluatedData.Magnitude;
	}

	// Get the Target actor, which should be our owner
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	AYogCharacterBase* TargetCharacter = nullptr;
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		TargetCharacter = Cast<AYogCharacterBase>(TargetActor);
	}



	if (Data.EvaluatedData.Attribute == GetDamagePhysicalAttribute())
	{
		AActor* SourceActor = nullptr;
		AController* SourceController = nullptr;
		AYogCharacterBase* SourceCharacter = nullptr;

		if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
		{
			SourceActor = Source->AbilityActorInfo->AvatarActor.Get();
			SourceController = Source->AbilityActorInfo->PlayerController.Get();
			if (SourceController == nullptr && SourceActor != nullptr)
			{
				if (APawn* Pawn = Cast<APawn>(SourceActor))
				{
					SourceController = Pawn->GetController();
				}
			}

			// Use the controller to find the source pawn
			if (SourceController)
			{
				SourceCharacter = Cast<AYogCharacterBase>(SourceController->GetPawn());
			}
			else
			{
				SourceCharacter = Cast<AYogCharacterBase>(SourceActor);
			}

			// Set the causer actor based on context if it's set
			if (Context.GetEffectCauser())
			{
				SourceActor = Context.GetEffectCauser();
			}
		}

		const float LocalDamageDone = GetDamagePhysical();
		SetDamagePhysical(0.f);
		if (LocalDamageDone > 0)
		{
			// Apply the health change and then clamp it
			const float OldHealth = TargetCharacter->BaseAttributeSet->GetHealth();
			TargetCharacter->BaseAttributeSet->SetHealth(FMath::Clamp(OldHealth - LocalDamageDone, 0.0f, TargetCharacter->BaseAttributeSet->GetMaxHealth()));
			UYogAbilitySystemComponent* ASC = TargetCharacter->GetASC();
			if (ASC)
			{
				//UYogAbilitySystemComponent* SourceASC, float Damage
				ASC->ReceiveDamage(ASC, GetDamagePhysical());
				float percent = TargetCharacter->BaseAttributeSet->GetHealth() / TargetCharacter->BaseAttributeSet->GetMaxHealth();
				TargetCharacter->OnCharacterHealthUpdate.Broadcast(percent);
				// This is proper damage
			}
		}

	}
}



