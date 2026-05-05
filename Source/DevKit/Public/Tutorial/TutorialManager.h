#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tutorial/TutorialHintDataAsset.h"
#include "UI/GameDialogWidget.h"
#include "TutorialManager.generated.h"

class UTutorialPopupWidget;
class UTutorialRegistryDA;
class AYogPlayerControllerBase;
class UYogSaveGame;

UCLASS()
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

	// LevelFlow entry: show a registered tutorial popup directly without state validation.
	bool ShowByEventID(FName EventID, APlayerController* PC, bool bPauseGame = true);

	// LevelFlow entry: show inline tutorial pages without requiring a DialogContentDA asset.
	bool ShowInlinePages(const TArray<FTutorialPage>& Pages, APlayerController* PC, bool bPauseGame = true);

	ETutorialState GetState() const { return State; }
	bool IsPopupShowing() const { return bPopupShowing; }

	void NotifyPopupClosed();
	void ForceClosePopup();

	FSimpleMulticastDelegate OnPopupClosed;

	virtual void Deinitialize() override;

	// Stage order for persisted enum values. Extend here instead of relying on enum numeric order.
	int32 StageRank(ETutorialState S) const;
	bool HasPassedStage(ETutorialState Required) const;

private:
	ETutorialState State = ETutorialState::NeedWeaponTutorial;

	TWeakObjectPtr<UTutorialPopupWidget> PopupWidget;

	UPROPERTY()
	TObjectPtr<UTutorialRegistryDA> Registry;

	bool bPopupShowing = false;

	FTimerHandle DelayHandle;
	FTSTicker::FDelegateHandle DilationTickerHandle;

	void DoShowWeaponPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC);
	void DoShowPostCombatPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC);

	void SaveState();
};
