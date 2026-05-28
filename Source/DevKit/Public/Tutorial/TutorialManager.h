#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tutorial/TutorialHintDataAsset.h"
#include "UI/GameDialogWidget.h"
#include "TutorialManager.generated.h"

class UTutorialPopupWidget;
class UTutorialRegistryDA;
class AYogPlayerControllerBase;
class ULevelInfoPopupDA;
class UYogSaveGame;

UCLASS(Config=Game)
class DEVKIT_API UTutorialManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void Init(UTutorialPopupWidget* InWidget, UTutorialRegistryDA* InRegistry);

	void TryWeaponTutorial(AYogPlayerControllerBase* PC);

	// Compatibility entry used after loot selection. In 512 this shows the first reward-card tutorial.
	void TryPostCombatTutorial(AYogPlayerControllerBase* PC);

	void LoadFromSave(UYogSaveGame* Save);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void TryFirstRuneTutorial(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void TryBackpackTutorial(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void TryHeatPhaseTutorial(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void TryCardLinkTutorial(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void NotifyLinkCardEnteredDeck(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	bool TryShowPendingLinkCardTutorial(APlayerController* PC);

	// LevelFlow entry: show a registered tutorial popup directly without state validation.
	bool ShowByEventID(FName EventID, APlayerController* PC, bool bPauseGame = true);

	// LevelFlow entry: show inline tutorial pages without requiring a DialogContentDA asset.
	bool ShowInlinePages(const TArray<FTutorialPage>& Pages, APlayerController* PC, bool bPauseGame = true);

	// 事件驱动一次性提示：HintTag 未展示过则展示 EventID 对应弹窗，并将 HintTag 记入存档 ShownPopupKeys。
	// 不依赖 ETutorialState 顺序，适合背包打开/卡牌获得等孤立触发点。
	// 当前有弹窗正在显示时，提示进入队列，等 NotifyPopupClosed 后自动补发，不会丢失。
	UFUNCTION(BlueprintCallable, Category = "Tutorial|HintOnce")
	bool TryShowHintOnce(FGameplayTag HintTag, FName EventID, APlayerController* PC, bool bPauseGame = true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tutorial|HintOnce")
	bool HasShownHint(FGameplayTag HintTag) const;

	// 每次 NotifyPopupClosed 后自动调用，尝试发送队列中的下一条提示。
	void FlushPendingHints();

	ETutorialState GetState() const { return State; }
	bool IsPopupShowing() const { return bPopupShowing; }
	bool AreTutorialPopupsEnabled() const { return bTutorialPopupsEnabled; }

	void NotifyPopupClosed();
	void ForceClosePopup();

	FSimpleMulticastDelegate OnPopupClosed;

	virtual void Deinitialize() override;

	// Stage order for persisted enum values. Extend here instead of relying on enum numeric order.
	int32 StageRank(ETutorialState S) const;
	bool HasPassedStage(ETutorialState Required) const;

	static FName ResolveLinkCardTutorialEventIdForTest(const UTutorialRegistryDA* InRegistry);
	static UTutorialRegistryDA* ResolveTutorialRegistryForTest(UTutorialRegistryDA* InRegistry);
	static bool IsDirectEventTutorialAllowedForTest(FName EventID);

private:
	UPROPERTY(Config)
	bool bTutorialPopupsEnabled = false;

	void MarkHintShown(FGameplayTag HintTag);

	struct FPendingHint
	{
		FGameplayTag HintTag;
		FName EventID;
		TWeakObjectPtr<APlayerController> PC;
		bool bPauseGame = true;
	};
	TArray<FPendingHint> PendingHints;

	ETutorialState State = ETutorialState::NeedWeaponTutorial;

	TWeakObjectPtr<UTutorialPopupWidget> PopupWidget;

	UPROPERTY()
	TObjectPtr<UTutorialRegistryDA> Registry;

	UPROPERTY()
	TArray<TObjectPtr<ULevelInfoPopupDA>> TransientInfoPopups;

	bool bPopupShowing = false;
	bool bLinkCardBackpackTutorialPending = false;

	FTimerHandle DelayHandle;
	FTSTicker::FDelegateHandle DilationTickerHandle;
	bool bDilationVisualActive = false;

	void DoShowWeaponPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC);
	void DoShowPostCombatPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC);
	void EndDilationVisualIfActive();
	static UTutorialRegistryDA* ResolveTutorialRegistry(UTutorialRegistryDA* InRegistry);
	static bool IsDirectEventTutorialAllowed(FName EventID);
	FName ResolveLinkCardTutorialEventId() const;
	void ShowLinkCardBackpackPrompt(APlayerController* PC);

	void SaveState();
};
