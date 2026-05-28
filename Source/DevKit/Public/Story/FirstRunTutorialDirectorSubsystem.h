#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Story/StoryNextRoomPlanTypes.h"
#include "FirstRunTutorialDirectorSubsystem.generated.h"

class APlayerCharacterBase;
class APlayerController;
class AYogGameMode;
class URuneDataAsset;

UENUM(BlueprintType)
enum class EFirstRunTutorialStage : uint8
{
	None UMETA(DisplayName = "None"),
	GoldRoomCleared UMETA(DisplayName = "Gold Room Cleared"),
	BuffCardRoom UMETA(DisplayName = "Buff Card Room"),
	MoonlightRoom UMETA(DisplayName = "Moonlight Room"),
	TransitionRoom01 UMETA(DisplayName = "Transition Room 01"),
	TransitionRoom02 UMETA(DisplayName = "Transition Room 02"),
	PrayerRoom UMETA(DisplayName = "Prayer Room"),
	ForcedSurvival UMETA(DisplayName = "Forced Survival"),
	Completed UMETA(DisplayName = "Completed"),
};

UCLASS()
class DEVKIT_API UFirstRunTutorialDirectorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Story|FirstRunTutorial")
	void SetStage(EFirstRunTutorialStage InStage);

	UFUNCTION(BlueprintPure, Category = "Story|FirstRunTutorial")
	EFirstRunTutorialStage GetStage() const { return Stage; }

	UFUNCTION(BlueprintCallable, Category = "Story|FirstRunTutorial")
	void HandleArrangementPhase(AYogGameMode* GameMode);

	UFUNCTION(BlueprintCallable, Category = "Story|FirstRunTutorial")
	void HandleRewardRuneAdded(URuneDataAsset* RuneAsset, APlayerCharacterBase* Player);

	UFUNCTION(BlueprintCallable, Category = "Story|FirstRunTutorial")
	void HandleSacrificeConfirmed(URuneDataAsset* GrantedRune, APlayerCharacterBase* Player);

	UFUNCTION(BlueprintPure, Category = "Story|FirstRunTutorial")
	URuneDataAsset* ResolveSacrificeRewardOverride(URuneDataAsset* DefaultRune) const;

	UFUNCTION(BlueprintPure, Category = "Story|FirstRunTutorial")
	bool IsPrayerSacrificeOverrideActive() const;

	UFUNCTION(BlueprintPure, Category = "Story|FirstRunTutorial")
	bool ShouldHandleScriptedDefeatDeath() const;

	UFUNCTION(BlueprintCallable, Category = "Story|FirstRunTutorial")
	void HandleScriptedDefeatDeath(AYogGameMode* GameMode);

	/** 背包首次打开时调用（保留接口，触发逻辑已移至月光拾取时）。 */
	void HandleFirstBackpackOpened(APlayerController* PC);

	/** 教程关卡中玩家死亡（祈祷室前）：重置 Director 阶段及 session 标记，教程本局继续活跃，下局从头规划。 */
	void HandleTutorialRestartForDeath();

	UFUNCTION(BlueprintPure, Category = "Story|FirstRunTutorial")
	bool ShouldRestartTutorialOnDeath() const;

	static bool BuildDefaultNextRoomPlanForStage(EFirstRunTutorialStage Stage, FStoryNextRoomPlan& OutPlan);
	static void BuildDefaultPostTutorialDeck(TArray<URuneDataAsset*>& OutDeck);
	static bool IsRuneAtPath(const URuneDataAsset* Rune, const TCHAR* Path);
	static URuneDataAsset* LoadFirstRunFinisherRune();
	static URuneDataAsset* ResolveSacrificeRewardForStage(EFirstRunTutorialStage Stage, bool bFirstRunTutorialActive, URuneDataAsset* DefaultRune);

private:
	static EFirstRunTutorialStage GetNextStageAfterPlanning(EFirstRunTutorialStage PlanningStage);
	bool IsFirstRunTutorialActive() const;
	void BroadcastTutorialStoryEvent(FGameplayTag EventTag, APlayerCharacterBase* Player) const;
	void ShowMoonlightPickupTutorial(APlayerController* PC);

	UPROPERTY()
	EFirstRunTutorialStage Stage = EFirstRunTutorialStage::None;

	bool bMoonlightObtained = false;
	bool bMoonlightTutorialShown = false;
};
