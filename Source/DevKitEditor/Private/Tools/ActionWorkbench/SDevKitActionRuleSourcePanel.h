#pragma once

#include "CoreMinimal.h"
#include "ActionRules/DevKitActionRuleSet.h"
#include "Widgets/SCompoundWidget.h"

class SBox;

/** Read-only candidate model. Validation never mounts a profile or modifies settings. */
class FDevKitActionRuleSourceDraft
{
public:
	void SetProfile(const FString& InProfile);
	bool Validate(FString& Error);
	bool RevalidateForApply(FString& Error);
	const FString& GetProfile() const { return Profile; }
	TSharedPtr<const FDevKitActionRuleSet> GetCandidate() const { return Candidate; }
private:
	FString Profile;
	TSharedPtr<FDevKitActionRuleSet> Candidate;
};

/** Optional external authoring, explicit mount, read-only preview. No automatic reload. */
class SDevKitActionRuleSourcePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitActionRuleSourcePanel) {}
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs);
private:
	friend struct FDevKitActionRuleSourcePanelTestAccessor;
	FReply BrowseProfile();
	FReply ValidateCandidate();
	FReply SaveMountSettings();
	FReply EvaluateCandidate();
	FReply RefreshWorldStatus();
	void RefreshRuleRows();
	void InvalidateSimulation();
	static bool IsPlaying();
	FDevKitActionRuleSourceDraft Draft;
	bool bDraftEnabled = false;
	FString RequestInput = TEXT("Character.State.Skill.Attack");
	FString CurrentInput = TEXT("Character.State.Movement.Dash");
	FString SubjectInput = TEXT("player");
	FString ValuesInput;
	FText Status, Evaluation, WorldStatus;
	TSharedPtr<SBox> RuleRows;
};
