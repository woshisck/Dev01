#include "BuffFlow/Nodes/BFNode_CombatCardContext.h"

#include "BuffFlow/BuffFlowComponent.h"

namespace
{
	const FName PinHasContext(TEXT("bHasContext"));
	const FName PinEffectMultiplier(TEXT("EffectMultiplier"));
	const FName PinComboIndex(TEXT("ComboIndex"));
	const FName PinComboBonusStacks(TEXT("ComboBonusStacks"));
	const FName PinAttackDamage(TEXT("AttackDamage"));
	const FName PinFromLink(TEXT("bFromLink"));
	const FName PinForwardLink(TEXT("bForwardLink"));
	const FName PinBackwardLink(TEXT("bBackwardLink"));
	const FName PinPendingBackwardLink(TEXT("bPendingBackwardLink"));
	const FName PinSourceCardFinisher(TEXT("bSourceCardFinisher"));
	const FName PinComboActionFinisher(TEXT("bComboActionFinisher"));
	const FName PinCardIdTag(TEXT("CardIdTag"));
	const FName PinCardEffectTags(TEXT("CardEffectTags"));
	const FName PinLinkedSourceCardIdTag(TEXT("LinkedSourceCardIdTag"));
	const FName PinLinkedSourceEffectTags(TEXT("LinkedSourceEffectTags"));
	const FName PinLinkedTargetCardIdTag(TEXT("LinkedTargetCardIdTag"));
	const FName PinLinkedTargetEffectTags(TEXT("LinkedTargetEffectTags"));
	const FName PinAbilityTag(TEXT("AbilityTag"));
	const FName PinComboTags(TEXT("ComboTags"));
	const FName PinActionType(TEXT("ActionType"));
	const FName PinCardType(TEXT("CardType"));

	bool MatchesBoolRequirement(const bool bValue, const EBFCombatCardBoolRequirement Requirement)
	{
		switch (Requirement)
		{
		case EBFCombatCardBoolRequirement::RequireFalse:
			return !bValue;
		case EBFCombatCardBoolRequirement::RequireTrue:
			return bValue;
		case EBFCombatCardBoolRequirement::Ignore:
		default:
			return true;
		}
	}

	bool MatchesIdRequirement(const FGameplayTag& ActualTag, const FGameplayTagContainer& RequiredTags)
	{
		return RequiredTags.IsEmpty() || (ActualTag.IsValid() && RequiredTags.HasTag(ActualTag));
	}

	bool MatchesEffectRequirement(const FGameplayTagContainer& ActualTags, const FGameplayTagContainer& RequiredTags)
	{
		return RequiredTags.IsEmpty() || ActualTags.HasAll(RequiredTags);
	}

	bool IsAnyLinkTriggered(const FCombatCardResolveResult& Result)
	{
		return Result.bTriggeredLink || Result.bTriggeredForwardLink || Result.bTriggeredBackwardLink;
	}

	bool MatchesLinkRequirement(const FCombatCardResolveResult& Result, const EBFCombatCardLinkRequirement Requirement)
	{
		switch (Requirement)
		{
		case EBFCombatCardLinkRequirement::NoLink:
			return !IsAnyLinkTriggered(Result);
		case EBFCombatCardLinkRequirement::AnyLink:
			return IsAnyLinkTriggered(Result);
		case EBFCombatCardLinkRequirement::ForwardLink:
			return Result.bTriggeredForwardLink;
		case EBFCombatCardLinkRequirement::BackwardLink:
			return Result.bTriggeredBackwardLink || Result.bPendingBackwardLink;
		case EBFCombatCardLinkRequirement::Ignore:
		default:
			return true;
		}
	}
}

UBFNode_Pure_CombatCardContext::UBFNode_Pure_CombatCardContext(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Pure");
#endif
	OutputPins = {
		FFlowPin(PinHasContext, EFlowPinType::Bool),
		FFlowPin(PinEffectMultiplier, EFlowPinType::Float),
		FFlowPin(PinComboIndex, EFlowPinType::Int),
		FFlowPin(PinComboBonusStacks, EFlowPinType::Int),
		FFlowPin(PinAttackDamage, EFlowPinType::Float),
		FFlowPin(PinFromLink, EFlowPinType::Bool),
		FFlowPin(PinForwardLink, EFlowPinType::Bool),
		FFlowPin(PinBackwardLink, EFlowPinType::Bool),
		FFlowPin(PinPendingBackwardLink, EFlowPinType::Bool),
		FFlowPin(PinSourceCardFinisher, EFlowPinType::Bool),
		FFlowPin(PinComboActionFinisher, EFlowPinType::Bool),
		FFlowPin(PinCardIdTag, EFlowPinType::GameplayTag),
		FFlowPin(PinCardEffectTags, EFlowPinType::GameplayTagContainer),
		FFlowPin(PinLinkedSourceCardIdTag, EFlowPinType::GameplayTag),
		FFlowPin(PinLinkedSourceEffectTags, EFlowPinType::GameplayTagContainer),
		FFlowPin(PinLinkedTargetCardIdTag, EFlowPinType::GameplayTag),
		FFlowPin(PinLinkedTargetEffectTags, EFlowPinType::GameplayTagContainer),
		FFlowPin(PinAbilityTag, EFlowPinType::GameplayTag),
		FFlowPin(PinComboTags, EFlowPinType::GameplayTagContainer),
		FFlowPin(PinActionType, EFlowPinType::Enum, StaticEnum<ECardRequiredAction>()),
		FFlowPin(PinCardType, EFlowPinType::Enum, StaticEnum<ECombatCardType>()),
	};
}

bool UBFNode_Pure_CombatCardContext::TryGetContext(FCombatCardEffectContext& OutContext) const
{
	if (const UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		if (BFC->HasCombatCardEffectContext())
		{
			OutContext = BFC->GetLastCombatCardEffectContext();
			return true;
		}
	}

	OutContext = FCombatCardEffectContext();
	return false;
}

FFlowDataPinResult_Bool UBFNode_Pure_CombatCardContext::TrySupplyDataPinAsBool_Implementation(const FName& PinName) const
{
	FCombatCardEffectContext Context;
	const bool bHasContext = TryGetContext(Context);
	if (PinName == PinHasContext)
	{
		return FFlowDataPinResult_Bool(bHasContext);
	}

	if (!bHasContext)
	{
		return FFlowDataPinResult_Bool(false);
	}

	if (PinName == PinFromLink)
	{
		return FFlowDataPinResult_Bool(Context.bFromLink);
	}
	if (PinName == PinForwardLink)
	{
		return FFlowDataPinResult_Bool(Context.ResolveResult.bTriggeredForwardLink);
	}
	if (PinName == PinBackwardLink)
	{
		return FFlowDataPinResult_Bool(Context.ResolveResult.bTriggeredBackwardLink);
	}
	if (PinName == PinPendingBackwardLink)
	{
		return FFlowDataPinResult_Bool(Context.ResolveResult.bPendingBackwardLink);
	}
	if (PinName == PinSourceCardFinisher)
	{
		return FFlowDataPinResult_Bool(Context.SourceCard.Config.CardType == ECombatCardType::Finisher);
	}
	if (PinName == PinComboActionFinisher)
	{
		return FFlowDataPinResult_Bool(Context.bIsComboFinisher);
	}

	return FFlowDataPinResult_Bool();
}

FFlowDataPinResult_Int UBFNode_Pure_CombatCardContext::TrySupplyDataPinAsInt_Implementation(const FName& PinName) const
{
	FCombatCardEffectContext Context;
	if (!TryGetContext(Context))
	{
		return FFlowDataPinResult_Int(0);
	}

	if (PinName == PinComboIndex)
	{
		return FFlowDataPinResult_Int(Context.ComboIndex);
	}
	if (PinName == PinComboBonusStacks)
	{
		return FFlowDataPinResult_Int(Context.ComboBonusStacks);
	}

	return FFlowDataPinResult_Int();
}

FFlowDataPinResult_Float UBFNode_Pure_CombatCardContext::TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const
{
	FCombatCardEffectContext Context;
	if (!TryGetContext(Context))
	{
		return FFlowDataPinResult_Float(PinName == PinEffectMultiplier ? 1.0 : 0.0);
	}

	if (PinName == PinEffectMultiplier)
	{
		return FFlowDataPinResult_Float(Context.EffectMultiplier);
	}
	if (PinName == PinAttackDamage)
	{
		return FFlowDataPinResult_Float(Context.ActionContext.AttackDamage);
	}

	return FFlowDataPinResult_Float();
}

FFlowDataPinResult_GameplayTag UBFNode_Pure_CombatCardContext::TrySupplyDataPinAsGameplayTag_Implementation(const FName& PinName) const
{
	FCombatCardEffectContext Context;
	if (!TryGetContext(Context))
	{
		return FFlowDataPinResult_GameplayTag(FGameplayTag::EmptyTag);
	}

	if (PinName == PinCardIdTag)
	{
		return FFlowDataPinResult_GameplayTag(Context.SourceCard.Config.CardIdTag);
	}
	if (PinName == PinLinkedSourceCardIdTag)
	{
		return FFlowDataPinResult_GameplayTag(Context.ResolveResult.LinkedSourceCard.Config.CardIdTag);
	}
	if (PinName == PinLinkedTargetCardIdTag)
	{
		return FFlowDataPinResult_GameplayTag(Context.ResolveResult.LinkedTargetCard.Config.CardIdTag);
	}
	if (PinName == PinAbilityTag)
	{
		return FFlowDataPinResult_GameplayTag(Context.AbilityTag);
	}

	return FFlowDataPinResult_GameplayTag();
}

FFlowDataPinResult_GameplayTagContainer UBFNode_Pure_CombatCardContext::TrySupplyDataPinAsGameplayTagContainer_Implementation(const FName& PinName) const
{
	FCombatCardEffectContext Context;
	if (!TryGetContext(Context))
	{
		return FFlowDataPinResult_GameplayTagContainer(FGameplayTagContainer());
	}

	if (PinName == PinCardEffectTags)
	{
		return FFlowDataPinResult_GameplayTagContainer(Context.SourceCard.Config.CardEffectTags);
	}
	if (PinName == PinLinkedSourceEffectTags)
	{
		return FFlowDataPinResult_GameplayTagContainer(Context.ResolveResult.LinkedSourceCard.Config.CardEffectTags);
	}
	if (PinName == PinLinkedTargetEffectTags)
	{
		return FFlowDataPinResult_GameplayTagContainer(Context.ResolveResult.LinkedTargetCard.Config.CardEffectTags);
	}
	if (PinName == PinComboTags)
	{
		return FFlowDataPinResult_GameplayTagContainer(Context.ComboTags);
	}

	return FFlowDataPinResult_GameplayTagContainer();
}

FFlowDataPinResult_Enum UBFNode_Pure_CombatCardContext::TrySupplyDataPinAsEnum_Implementation(const FName& PinName) const
{
	FCombatCardEffectContext Context;
	if (!TryGetContext(Context))
	{
		return FFlowDataPinResult_Enum();
	}

	if (PinName == PinActionType)
	{
		return FFlowDataPinResult_Enum::BuildResultFromNativeEnumValue(Context.ActionContext.ActionType);
	}
	if (PinName == PinCardType)
	{
		return FFlowDataPinResult_Enum::BuildResultFromNativeEnumValue(Context.SourceCard.Config.CardType);
	}

	return FFlowDataPinResult_Enum();
}

UBFNode_CombatCardContextBranch::UBFNode_CombatCardContextBranch(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Combat Card");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Pass")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_CombatCardContextBranch::ExecuteBuffFlowInput(const FName& PinName)
{
	const UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!BFC || !BFC->HasCombatCardEffectContext())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	TriggerOutput(MatchesContext(BFC->GetLastCombatCardEffectContext()) ? TEXT("Pass") : TEXT("Failed"), true);
}

bool UBFNode_CombatCardContextBranch::MatchesContext(const FCombatCardEffectContext& Context) const
{
	const FCombatCardResolveResult& Result = Context.ResolveResult;
	const FCombatCardConfig& SourceConfig = Context.SourceCard.Config;
	const FCombatCardConfig& LinkedSourceConfig = Result.LinkedSourceCard.Config;
	const FCombatCardConfig& LinkedTargetConfig = Result.LinkedTargetCard.Config;

	if (!MatchesBoolRequirement(SourceConfig.CardType == ECombatCardType::Finisher, SourceCardIsFinisher))
	{
		return false;
	}
	if (!MatchesBoolRequirement(Context.bIsComboFinisher, ComboActionIsFinisher))
	{
		return false;
	}
	if (!MatchesLinkRequirement(Result, LinkRequirement))
	{
		return false;
	}
	if (!RequiredSourceCardTypes.IsEmpty() && !RequiredSourceCardTypes.Contains(SourceConfig.CardType))
	{
		return false;
	}
	if (!MatchesIdRequirement(SourceConfig.CardIdTag, RequiredSourceCardIdTags)
		|| !MatchesEffectRequirement(SourceConfig.CardEffectTags, RequiredSourceCardEffectTags))
	{
		return false;
	}
	if (!MatchesIdRequirement(LinkedSourceConfig.CardIdTag, RequiredLinkedSourceCardIdTags)
		|| !MatchesEffectRequirement(LinkedSourceConfig.CardEffectTags, RequiredLinkedSourceCardEffectTags))
	{
		return false;
	}
	if (!MatchesIdRequirement(LinkedTargetConfig.CardIdTag, RequiredLinkedTargetCardIdTags)
		|| !MatchesEffectRequirement(LinkedTargetConfig.CardEffectTags, RequiredLinkedTargetCardEffectTags))
	{
		return false;
	}
	if (!RequiredComboTags.IsEmpty() && !Context.ComboTags.HasAll(RequiredComboTags))
	{
		return false;
	}
	if (RequiredAction != ECardRequiredAction::Any && Context.ActionContext.ActionType != RequiredAction)
	{
		return false;
	}

	return true;
}
