#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DevKitActionRuleSet.generated.h"

class FDevKitLuaRuleVM;

USTRUCT(BlueprintType)
struct DEVKIT_API FDevKitActionRuleDecision
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) bool bBlocked = false;
	UPROPERTY(BlueprintReadOnly) bool bError = false;
	UPROPERTY(BlueprintReadOnly) FString ProfileId;
	UPROPERTY(BlueprintReadOnly) FString Revision;
	UPROPERTY(BlueprintReadOnly) TArray<FString> MatchedRules;
	UPROPERTY(BlueprintReadOnly) TArray<FString> Reasons;
	UPROPERTY(BlueprintReadOnly) FGameplayTagContainer CancelAbilityTags;
};

struct DEVKIT_API FDevKitActionDefinition
{
	FString Id;
	FString DisplayName;
	FGameplayTagContainer AbilityTags;
	FGameplayTagContainer StateTags;
};

struct DEVKIT_API FDevKitActionRule
{
	FString Id, Subject, Current, Request, Effect, Condition, Reason;
	bool bEnabled = true;
};

/** Immutable validated snapshot. Evaluation never edits tags, assets or abilities. */
class DEVKIT_API FDevKitActionRuleSet
{
public:
	static TSharedPtr<FDevKitActionRuleSet> LoadFile(const FString& Filename, FString& Error);
	static TSharedPtr<FDevKitActionRuleSet> Parse(const FString& Json, const FString& Lua, FString& Error);
	FDevKitActionRuleDecision Evaluate(const FGameplayTagContainer& RequestTags,
		const FGameplayTagContainer& OwnedTags, const FString& Subject, const TMap<FString, double>& Values) const;
	FString Id, Revision, SourceFile;
	TArray<FDevKitActionDefinition> Actions;
	TArray<FDevKitActionRule> Rules;
private:
	TSharedPtr<FDevKitLuaRuleVM> VM;
	TMap<FString, int32> ActionIndices;
	TMap<FString, TArray<int32>> RulesByRequest;
};
