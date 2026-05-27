#include "Story/StoryRuleSetSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "Story/StoryRuleSetDA.h"
#include "UObject/Package.h"

namespace StoryRuleSetSetup
{
	const FString RuleRoot = TEXT("/Game/Story/Rules");

	FString ToObjectPath(const FString& PackagePath)
	{
		return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
	}

	bool PackageExists(const FString& PackagePath)
	{
		FString ExistingPackageFile;
		return FPackageName::DoesPackageExist(PackagePath, &ExistingPackageFile);
	}

	template <typename T>
	T* LoadAssetByPackagePath(const FString& PackagePath)
	{
		if (T* Existing = FindObject<T>(nullptr, *ToObjectPath(PackagePath)))
		{
			return Existing;
		}
		if (!PackageExists(PackagePath))
		{
			return nullptr;
		}
		return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
	}

	FGameplayTag Tag(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName), false);
	}

	FStoryCondition NotHasSaveFlag(const TCHAR* FlagTag)
	{
		FStoryCondition Condition;
		Condition.Type = EStoryConditionType::HasFlag;
		Condition.bInvert = true;
		Condition.FlagScope = EStoryFlagScope::Save;
		Condition.FlagTag = Tag(FlagTag);
		return Condition;
	}

	FStoryAction SetSaveFlag(const TCHAR* FlagTag)
	{
		FStoryAction Action;
		Action.Type = EStoryActionType::SetFlag;
		Action.FlagScope = EStoryFlagScope::Save;
		Action.FlagTag = Tag(FlagTag);
		return Action;
	}

	FStoryAction ShowHint(const TCHAR* Body, float Duration = 3.0f)
	{
		FStoryAction Action;
		Action.Type = EStoryActionType::ShowInfoHint;
		Action.HintText = FText::FromString(Body);
		Action.HintDuration = Duration;
		return Action;
	}

	FStoryAction ShowTutorial(const TCHAR* EventId)
	{
		FStoryAction Action;
		Action.Type = EStoryActionType::ShowTutorialPopup;
		Action.TutorialEventId = FName(EventId);
		Action.bPauseGame = true;
		return Action;
	}

	FStoryAction SetQuest(const TCHAR* QuestTag, const TCHAR* Text, const TCHAR* SourceTag)
	{
		FStoryAction Action;
		Action.Type = EStoryActionType::SetQuestTask;
		Action.QuestTaskId = Tag(QuestTag);
		Action.QuestTaskText = FText::FromString(Text);
		Action.QuestSourceTag = Tag(SourceTag);
		return Action;
	}

	FStoryAction CompleteQuest(const TCHAR* QuestTag)
	{
		FStoryAction Action;
		Action.Type = EStoryActionType::CompleteQuestTask;
		Action.QuestTaskId = Tag(QuestTag);
		return Action;
	}

	FStoryRule MakeRule(const TCHAR* RuleId, const TCHAR* EventTag, int32 Priority, EStoryRuleFirePolicy FirePolicy)
	{
		FStoryRule Rule;
		Rule.RuleId = FName(RuleId);
		Rule.TriggerEventTag = Tag(EventTag);
		Rule.Priority = Priority;
		Rule.FirePolicy = FirePolicy;
		return Rule;
	}

	UStoryRuleSetDA* LoadOrCreateRuleSet(const FString& AssetName, bool bDryRun, TArray<UPackage*>& DirtyPackages)
	{
		const FString PackagePath = RuleRoot + TEXT("/") + AssetName;
		if (UStoryRuleSetDA* Existing = LoadAssetByPackagePath<UStoryRuleSetDA>(PackagePath))
		{
			return Existing;
		}

		if (bDryRun)
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		if (!Package)
		{
			return nullptr;
		}

		UStoryRuleSetDA* RuleSet = NewObject<UStoryRuleSetDA>(
			Package,
			UStoryRuleSetDA::StaticClass(),
			*FPackageName::GetLongPackageAssetName(PackagePath),
			RF_Public | RF_Standalone | RF_Transactional);

		if (RuleSet)
		{
			FAssetRegistryModule::AssetCreated(RuleSet);
			RuleSet->MarkPackageDirty();
			DirtyPackages.AddUnique(RuleSet->GetPackage());
		}
		return RuleSet;
	}

	TArray<FStoryRule> BuildMemoryTutorialRules()
	{
		TArray<FStoryRule> Rules;

		FStoryRule Started = MakeRule(
			TEXT("MemoryTutorial.Started"),
			TEXT("Story.Event.MemoryTutorial.Started"),
			100,
			EStoryRuleFirePolicy::OncePerSave);
		Started.Conditions.Add(NotHasSaveFlag(TEXT("Story.Flag.MemoryTutorial.Started")));
		Started.Actions.Add(SetSaveFlag(TEXT("Story.Flag.MemoryTutorial.Started")));
		Started.Actions.Add(SetQuest(
			TEXT("Story.Quest.MemoryTutorial"),
			TEXT("找回这段战斗记忆"),
			TEXT("Story.Source.System")));
		Rules.Add(Started);

		FStoryRule Failed = MakeRule(
			TEXT("MemoryTutorial.PlayerFailed"),
			TEXT("Story.Event.MemoryTutorial.PlayerFailed"),
			90,
			EStoryRuleFirePolicy::Always);
		Failed.Actions.Add(ShowHint(TEXT("你已回到锚点，再次尝试"), 3.0f));
		Rules.Add(Failed);

		FStoryRule Completed = MakeRule(
			TEXT("MemoryTutorial.Completed"),
			TEXT("Story.Event.MemoryTutorial.Completed"),
			100,
			EStoryRuleFirePolicy::OncePerSave);
		Completed.Conditions.Add(NotHasSaveFlag(TEXT("Story.Flag.MemoryTutorial.Completed")));
		Completed.Actions.Add(SetSaveFlag(TEXT("Story.Flag.MemoryTutorial.Completed")));
		Completed.Actions.Add(CompleteQuest(TEXT("Story.Quest.MemoryTutorial")));
		Completed.Actions.Add(SetQuest(
			TEXT("Story.Quest.Main"),
			TEXT("进入第一局，寻找第一枚真正属于你的符文"),
			TEXT("Story.Source.Codex")));
		Rules.Add(Completed);

		return Rules;
	}

	TArray<FStoryRule> BuildFirstRunRules()
	{
		TArray<FStoryRule> Rules;

		FStoryRule Started = MakeRule(
			TEXT("FirstRun.Started"),
			TEXT("Story.Event.FirstRun.Started"),
			100,
			EStoryRuleFirePolicy::OncePerSave);
		Started.Actions.Add(SetQuest(
			TEXT("Story.Quest.Main"),
			TEXT("进入牢狱深处，取得第一枚符文"),
			TEXT("Story.Source.Codex")));
		Rules.Add(Started);

		FStoryRule FirstRune = MakeRule(
			TEXT("FirstRun.FirstRuneObtained"),
			TEXT("Story.Event.FirstRun.FirstRuneObtained"),
			100,
			EStoryRuleFirePolicy::OncePerSave);
		FirstRune.Conditions.Add(NotHasSaveFlag(TEXT("Story.Flag.FirstRune.Obtained")));
		FirstRune.Actions.Add(SetSaveFlag(TEXT("Story.Flag.FirstRune.Obtained")));
		FirstRune.Actions.Add(SetSaveFlag(TEXT("Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.heavy_card_obtained")));
		FirstRune.Actions.Add(ShowTutorial(TEXT("tutorial_first_rune")));
		FirstRune.Actions.Add(SetQuest(
			TEXT("Story.Quest.Main"),
			TEXT("打开背包查看你的符文"),
			TEXT("Story.Source.Codex")));
		Rules.Add(FirstRune);

		FStoryRule LegacyReward = MakeRule(
			TEXT("FirstRun.LegacyRewardToDeck"),
			TEXT("Tutorial.RewardToDeck"),
			90,
			EStoryRuleFirePolicy::OncePerSave);
		LegacyReward.Conditions.Add(NotHasSaveFlag(TEXT("Story.Flag.FirstRune.Obtained")));
		LegacyReward.Actions.Add(SetSaveFlag(TEXT("Story.Flag.FirstRune.Obtained")));
		LegacyReward.Actions.Add(SetSaveFlag(TEXT("Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.heavy_card_obtained")));
		LegacyReward.Actions.Add(SetQuest(
			TEXT("Story.Quest.Main"),
			TEXT("打开背包查看你的符文"),
			TEXT("Story.Source.Codex")));
		Rules.Add(LegacyReward);

		FStoryRule Backpack = MakeRule(
			TEXT("FirstRun.FirstBackpackOpened"),
			TEXT("Story.Event.FirstRun.FirstBackpackOpened"),
			100,
			EStoryRuleFirePolicy::OncePerSave);
		Backpack.Conditions.Add(NotHasSaveFlag(TEXT("Story.Flag.FirstBackpack.Opened")));
		Backpack.Actions.Add(SetSaveFlag(TEXT("Story.Flag.FirstBackpack.Opened")));
		Backpack.Actions.Add(ShowTutorial(TEXT("tutorial_backpack")));
		Backpack.Actions.Add(SetQuest(
			TEXT("Story.Quest.Main"),
			TEXT("整理符文后继续前进"),
			TEXT("Story.Source.Codex")));
		Rules.Add(Backpack);

		FStoryRule LegacyBackpack = MakeRule(
			TEXT("FirstRun.LegacyBackpackArrange"),
			TEXT("Tutorial.BackpackArrange"),
			90,
			EStoryRuleFirePolicy::OncePerSave);
		LegacyBackpack.Conditions.Add(NotHasSaveFlag(TEXT("Story.Flag.FirstBackpack.Opened")));
		LegacyBackpack.Actions.Add(SetSaveFlag(TEXT("Story.Flag.FirstBackpack.Opened")));
		LegacyBackpack.Actions.Add(SetQuest(
			TEXT("Story.Quest.Main"),
			TEXT("整理符文后继续前进"),
			TEXT("Story.Source.Codex")));
		Rules.Add(LegacyBackpack);

		return Rules;
	}

	TArray<FStoryRule> BuildHubOnboardingRules()
	{
		TArray<FStoryRule> Rules;

		FStoryRule FirstEnter = MakeRule(
			TEXT("Hub.FirstEntered"),
			TEXT("Story.Event.Hub.FirstEntered"),
			100,
			EStoryRuleFirePolicy::OncePerSave);
		FirstEnter.Conditions.Add(NotHasSaveFlag(TEXT("Story.Flag.Hub.FirstEntered")));
		FirstEnter.Actions.Add(SetSaveFlag(TEXT("Story.Flag.Hub.FirstEntered")));
		FirstEnter.Actions.Add(SetQuest(
			TEXT("Story.Quest.Main"),
			TEXT("和黑夜少女交谈"),
			TEXT("Story.Source.Codex")));
		Rules.Add(FirstEnter);

		FStoryRule FirstUpgrade = MakeRule(
			TEXT("Hub.MetaFirstUpgradePurchased"),
			TEXT("Story.Event.Hub.MetaFirstUpgradePurchased"),
			90,
			EStoryRuleFirePolicy::OncePerSave);
		FirstUpgrade.Conditions.Add(NotHasSaveFlag(TEXT("Story.Flag.Meta.FirstUpgradePurchased")));
		FirstUpgrade.Actions.Add(SetSaveFlag(TEXT("Story.Flag.Meta.FirstUpgradePurchased")));
		FirstUpgrade.Actions.Add(CompleteQuest(TEXT("Story.Quest.Main")));
		Rules.Add(FirstUpgrade);

		return Rules;
	}

	void WriteRuleSet(
		UStoryRuleSetDA* RuleSet,
		const TArray<FStoryRule>& Rules,
		TArray<UPackage*>& DirtyPackages)
	{
		if (!RuleSet)
		{
			return;
		}

		RuleSet->Modify();
		RuleSet->bEnabled = true;
		RuleSet->Rules = Rules;
		RuleSet->MarkPackageDirty();
		DirtyPackages.AddUnique(RuleSet->GetPackage());
	}
}

UStoryRuleSetSetupCommandlet::UStoryRuleSetSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UStoryRuleSetSetupCommandlet::Main(const FString& Params)
{
	using namespace StoryRuleSetSetup;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;

	UE_LOG(LogTemp, Display, TEXT("[StoryRuleSetSetup] Mode=%s Root=%s"), bDryRun ? TEXT("DryRun") : TEXT("Apply"), *RuleRoot);
	if (bDryRun)
	{
		UE_LOG(LogTemp, Display, TEXT("[StoryRuleSetSetup] Would create/update SR_MemoryTutorial, SR_FirstRun, SR_HubOnboarding."));
		return 0;
	}

	TArray<UPackage*> DirtyPackages;
	WriteRuleSet(
		LoadOrCreateRuleSet(TEXT("SR_MemoryTutorial"), bDryRun, DirtyPackages),
		BuildMemoryTutorialRules(),
		DirtyPackages);
	WriteRuleSet(
		LoadOrCreateRuleSet(TEXT("SR_FirstRun"), bDryRun, DirtyPackages),
		BuildFirstRunRules(),
		DirtyPackages);
	WriteRuleSet(
		LoadOrCreateRuleSet(TEXT("SR_HubOnboarding"), bDryRun, DirtyPackages),
		BuildHubOnboardingRules(),
		DirtyPackages);

	if (DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	UE_LOG(LogTemp, Display, TEXT("[StoryRuleSetSetup] Updated %d package(s)."), DirtyPackages.Num());
	return 0;
}
