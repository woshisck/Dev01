// Fill out your copyright notice in the Description page of Project Settings.


#include "YogAnimInstance.h"
#include "../Character/YogCharacterBase.h"
#include "../Character/YogCharacterMovementComponent.h"
#include "AbilitySystemGlobals.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UYogAnimInstance::UYogAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UYogAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);
	GameplayTagPropertyMap.Initialize(this, ASC);

}

EDataValidationResult UYogAnimInstance::IsDataValid(class FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);
	GameplayTagPropertyMap.IsDataValid(this, Context);
	return ((Context.GetNumErrors() > 0) ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}

void UYogAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* OwningActor = GetOwningActor())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor))
		{
			InitializeWithAbilitySystem(ASC);
		}
	}
}

void UYogAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const AYogCharacterBase* Character = Cast<AYogCharacterBase>(GetOwningActor());
	if (!Character)
	{
		return;
	}
	//[Sunchuankai] Update Ground info from movementComponent;
	//UYogCharacterMovementComponent* CharMoveComp = CastChecked<UYogCharacterMovementComponent>(Character->GetCharacterMovement());
	//const FLyraCharacterGroundInfo& GroundInfo = CharMoveComp->GetGroundInfo();
	//GroundDistance = GroundInfo.GroundDistance;
}
