#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/Nodes/BFNode_PureData.h"
#include "Component/CombatDeckComponent.h"
#include "Data/RuneDataAsset.h"
#include "Types/FlowDataPinResults.h"
#include "BFNode_CombatCardContext.generated.h"

UENUM(BlueprintType)
enum class EBFCombatCardBoolRequirement : uint8
{
	Ignore,
	RequireFalse,
	RequireTrue,
};

UENUM(BlueprintType)
enum class EBFCombatCardLinkRequirement : uint8
{
	Ignore,
	NoLink,
	AnyLink,
	ForwardLink,
	BackwardLink,
};

/**
 * Pure data supplier for combat-card FA context.
 *
 * This keeps attack, finisher, and link flows reading the same context values
 * instead of each feature reaching into CombatDeckComponent differently.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Combat Card Context", Keywords = "Card Context Info Finisher Link Forward Backward Combat Card Context Link Finisher Moonlight"))
class DEVKIT_API UBFNode_Pure_CombatCardContext : public UBFNode_PureData
{
	GENERATED_UCLASS_BODY()

	virtual FFlowDataPinResult_Bool TrySupplyDataPinAsBool_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Int TrySupplyDataPinAsInt_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Float TrySupplyDataPinAsFloat_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_GameplayTag TrySupplyDataPinAsGameplayTag_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_GameplayTagContainer TrySupplyDataPinAsGameplayTagContainer_Implementation(const FName& PinName) const override;
	virtual FFlowDataPinResult_Enum TrySupplyDataPinAsEnum_Implementation(const FName& PinName) const override;

private:
	bool TryGetContext(FCombatCardEffectContext& OutContext) const;
};

/**
 * Generic branch for combat-card context checks.
 *
 * Use this for moonlight forward/reversed links, finisher-only payloads, or
 * effect-tag based card logic without hardcoding card names in C++.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Combat Card Context Branch", Keywords = "Card Context Branch Condition Finisher Link Forward Backward Combat Card Context Branch Link Finisher Moonlight"))
class DEVKIT_API UBFNode_CombatCardContextBranch : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Combat Card|Context")
	EBFCombatCardBoolRequirement SourceCardIsFinisher = EBFCombatCardBoolRequirement::Ignore;

	UPROPERTY(EditAnywhere, Category = "Combat Card|Context")
	EBFCombatCardBoolRequirement ComboActionIsFinisher = EBFCombatCardBoolRequirement::Ignore;

	UPROPERTY(EditAnywhere, Category = "Combat Card|Context")
	EBFCombatCardLinkRequirement LinkRequirement = EBFCombatCardLinkRequirement::Ignore;

	UPROPERTY(EditAnywhere, Category = "Combat Card|Source")
	TArray<ECombatCardType> RequiredSourceCardTypes;

	UPROPERTY(EditAnywhere, Category = "Combat Card|Source", meta = (Categories = "Card.ID"))
	FGameplayTagContainer RequiredSourceCardIdTags;

	UPROPERTY(EditAnywhere, Category = "Combat Card|Source", meta = (Categories = "Card.Effect"))
	FGameplayTagContainer RequiredSourceCardEffectTags;

	UPROPERTY(EditAnywhere, Category = "Combat Card|Linked Source", meta = (Categories = "Card.ID"))
	FGameplayTagContainer RequiredLinkedSourceCardIdTags;

	UPROPERTY(EditAnywhere, Category = "Combat Card|Linked Source", meta = (Categories = "Card.Effect"))
	FGameplayTagContainer RequiredLinkedSourceCardEffectTags;

	UPROPERTY(EditAnywhere, Category = "Combat Card|Linked Target", meta = (Categories = "Card.ID"))
	FGameplayTagContainer RequiredLinkedTargetCardIdTags;

	UPROPERTY(EditAnywhere, Category = "Combat Card|Linked Target", meta = (Categories = "Card.Effect"))
	FGameplayTagContainer RequiredLinkedTargetCardEffectTags;

	UPROPERTY(EditAnywhere, Category = "Combat Card|Action", meta = (Categories = ""))
	FGameplayTagContainer RequiredComboTags;

	UPROPERTY(EditAnywhere, Category = "Combat Card|Action")
	ECardRequiredAction RequiredAction = ECardRequiredAction::Any;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	bool MatchesContext(const FCombatCardEffectContext& Context) const;
};
