#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tutorial/TutorialHintDataAsset.h"
#include "TutorialManager.generated.h"

class UTutorialPopupWidget;
class UDialogContentDA;
class AYogPlayerControllerBase;
class UYogSaveGame;

UCLASS()
class DEVKIT_API UTutorialManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void Init(UTutorialPopupWidget* InWidget, UDialogContentDA* InContentDA);

	void TryWeaponTutorial(AYogPlayerControllerBase* PC);
	void TryPostCombatTutorial(AYogPlayerControllerBase* PC);
	void LoadFromSave(UYogSaveGame* Save);

	// 供 LevelFlow 节点调用：按 EventID 直接显示教程弹窗（不检查 State）
	void ShowByEventID(FName EventID, APlayerController* PC);

	ETutorialState GetState() const { return State; }

	// 返回 true 表示教程弹窗正在显示（Tick 中用于屏蔽浮窗）
	bool IsPopupShowing() const;

private:
	ETutorialState State = ETutorialState::NeedWeaponTutorial;

	TWeakObjectPtr<UTutorialPopupWidget> PopupWidget;

	UPROPERTY()
	TObjectPtr<UDialogContentDA> ContentDA;

	FTimerHandle DelayHandle;

	// FTSTicker 句柄：用于在时间膨胀期间以真实时间计时
	FTSTicker::FDelegateHandle DilationTickerHandle;

	void DoShowWeaponPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC);
	void DoShowPostCombatPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC);

	void SaveState();
};
