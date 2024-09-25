// Copyright 2022 Mickael Daniel. All Rights Reserved.

#include "Animations/GSCNativeAnimInstance.h"

#include "AbilitySystemGlobals.h"

#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
EDataValidationResult UGSCNativeAnimInstance::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult SuperResult = Super::IsDataValid(Context);
	const EDataValidationResult Result = GameplayTagPropertyMap.IsDataValid(this, Context);
	return CombineDataValidationResults(SuperResult, Result);
}
#else
EDataValidationResult UGSCNativeAnimInstance::IsDataValid(TArray<FText>& ValidationErrors)
{
	const EDataValidationResult SuperResult = Super::IsDataValid(ValidationErrors);
	const EDataValidationResult Result = GameplayTagPropertyMap.IsDataValid(this, ValidationErrors);
	return CombineDataValidationResults(SuperResult, Result);
}
#endif
#endif

void UGSCNativeAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwningActor()))
	{
		InitializeWithAbilitySystem(ASC);
	}
}
