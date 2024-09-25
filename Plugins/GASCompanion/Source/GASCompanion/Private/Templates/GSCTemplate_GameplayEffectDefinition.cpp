// Copyright 2020 Mickael Daniel. All Rights Reserved.


#include "Templates/GSCTemplate_GameplayEffectDefinition.h"

#include "GSCLog.h"
#include "Runtime/Launch/Resources/Version.h"

UGSCTemplate_GameplayEffectDefinition::UGSCTemplate_GameplayEffectDefinition()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	bExecutePeriodicEffectOnApplication = true;
	PeriodicInhibitionPolicy = EGameplayEffectPeriodInhibitionRemovedPolicy::NeverReset;
	ChanceToApplyToTarget.SetValue(1.f);
	StackingType = EGameplayEffectStackingType::None;
	StackLimitCount = 0;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;
	bRequireModifierSuccessToTriggerCues = true;
}

void UGSCTemplate_GameplayEffectDefinition::PostInitProperties()
{
	UObject::PostInitProperties();

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	InheritableGameplayEffectTags.Added.Reset();
	InheritableGameplayEffectTags.Removed.Reset();
	InheritableOwnedTagsContainer.Added.Reset();
	InheritableOwnedTagsContainer.Removed.Reset();
	RemoveGameplayEffectsWithTags.Added.Reset();
	RemoveGameplayEffectsWithTags.Removed.Reset();
#else
	InheritableGameplayEffectTags.PostInitProperties();
	InheritableOwnedTagsContainer.PostInitProperties();
	RemoveGameplayEffectsWithTags.PostInitProperties();
#endif
}

#if WITH_EDITOR
void UGSCTemplate_GameplayEffectDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	const FProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
	if (PropertyThatChanged)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const UGameplayEffect* Parent = Cast<UGameplayEffect>(GetClass()->GetSuperClass()->GetDefaultObject());
		const FName PropName = PropertyThatChanged->GetFName();
		if (PropName == GET_MEMBER_NAME_CHECKED(UGSCTemplate_GameplayEffectDefinition, InheritableGameplayEffectTags))
		{
			InheritableGameplayEffectTags.UpdateInheritedTagProperties(Parent ? &Parent->InheritableGameplayEffectTags : NULL);
		}
		else if (PropName == GET_MEMBER_NAME_CHECKED(UGSCTemplate_GameplayEffectDefinition, InheritableOwnedTagsContainer))
		{
			InheritableOwnedTagsContainer.UpdateInheritedTagProperties(Parent ? &Parent->InheritableOwnedTagsContainer : NULL);
		}
		else if (PropName == GET_MEMBER_NAME_CHECKED(UGSCTemplate_GameplayEffectDefinition, RemoveGameplayEffectsWithTags))
		{
			RemoveGameplayEffectsWithTags.UpdateInheritedTagProperties(Parent ? &Parent->RemoveGameplayEffectsWithTags : NULL);
		}
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
}
#endif

void UGSCTemplate_GameplayEffectDefinition::CopyProperties(UGameplayEffect* CDO, UGSCTemplate_GameplayEffectDefinition* TemplateCDO)
{
	check(CDO);
	check(TemplateCDO);

	GSC_LOG(Verbose, TEXT("UGSCTemplate_GameplayEffectDefinition::CopyProperties for %s from %s"), *GetNameSafe(CDO), *GetNameSafe(TemplateCDO))
	UEngine::CopyPropertiesForUnrelatedObjects(TemplateCDO, CDO);
}
