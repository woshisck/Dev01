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
	void TryPostCombatTutorial(AYogPlayerControllerBase* PC);
	void LoadFromSave(UYogSaveGame* Save);

	// 新四段教程流（① 武器拾取走 LevelFlow，②③④ 由各业务点调用）
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void TryFirstRuneTutorial(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void TryBackpackTutorial(APlayerController* PC);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void TryHeatPhaseTutorial(APlayerController* PC);

	// 供 LevelFlow 节点调用：按 EventID 直接显示教程弹窗（不检查 State）
	// bPauseGame=false 用于纯信息提示（不暂停游戏）
	// 返回 true 表示弹窗确实显示了（用于调用方决定是否推进状态机）
	bool ShowByEventID(FName EventID, APlayerController* PC, bool bPauseGame = true);

	// 供 LevelFlow 节点调用：直接传入页面内容显示弹窗（不依赖 DA，内联填写）
	bool ShowInlinePages(const TArray<FTutorialPage>& Pages, APlayerController* PC, bool bPauseGame = true);

	ETutorialState GetState() const { return State; }

	// 返回 true 表示教程弹窗正在显示（Tick 中用于屏蔽浮窗）
	bool IsPopupShowing() const { return bPopupShowing; }

	// 由 GameDialogWidget 在关闭时调用，清除弹窗标志并广播委托
	void NotifyPopupClosed();

	// 强制立即关闭弹窗（跳过渐出动画），用于武器拾取等打断场景
	void ForceClosePopup();

	// 弹窗关闭时广播（供 LENode_ShowTutorial 等节点等待）
	FSimpleMulticastDelegate OnPopupClosed;

	// Subsystem 销毁时兜底恢复全局时间膨胀，避免 ticker 来不及恢复造成全局慢动作
	virtual void Deinitialize() override;

	// 阶段顺序判断（不依赖枚举数值大小，新增状态时只需扩展 StageRank）
	int32 StageRank(ETutorialState S) const;
	bool HasPassedStage(ETutorialState Required) const;

private:
	ETutorialState State = ETutorialState::NeedWeaponTutorial;

	TWeakObjectPtr<UTutorialPopupWidget> PopupWidget;

	UPROPERTY()
	TObjectPtr<UTutorialRegistryDA> Registry;

	// 弹窗是否正在显示（由 DoShowWeaponPopup/DoShowPostCombatPopup 置 true，NotifyPopupClosed 置 false）
	bool bPopupShowing = false;

	FTimerHandle DelayHandle;

	// FTSTicker 句柄：用于在时间膨胀期间以真实时间计时
	FTSTicker::FDelegateHandle DilationTickerHandle;

	void DoShowWeaponPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC);
	void DoShowPostCombatPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC);

	void SaveState();
};
