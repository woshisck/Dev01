#include "Tutorial/TutorialManager.h"
#include "UI/GameDialogWidget.h"
#include "UI/TutorialRegistryDA.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "Character/YogPlayerControllerBase.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/Ticker.h"

#define LOCTEXT_NAMESPACE "Tutorial"

// 旧两段流 EventID（保留兼容）
static const FName EventID_WeaponTutorial    = "WeaponTutorial";
static const FName EventID_PostCombatTutorial = "PostCombatTutorial";

// 新四段流 EventID（与 UTutorialRegistryDA::Entries 的 Key 一字不差）
// EventID_WeaponPickup 由 LevelFlow + LENode_ShowTutorial 直接引用裸字符串 "tutorial_weapon_pickup"，
// 此处保留作为单点真相，避免拼写漂移
static const FName EventID_WeaponPickup    = "tutorial_weapon_pickup";
static const FName EventID_FirstRune       = "tutorial_first_rune";
static const FName EventID_Backpack        = "tutorial_backpack";
static const FName EventID_ActivationZone  = "tutorial_activation_zone";

// ============================================================
//  Subsystem 生命周期
// ============================================================

void UTutorialManager::Deinitialize()
{
	// 兜底：Subsystem 销毁时强制恢复全局时间膨胀，
	// 防止 0.08 ticker 因玩家死亡/切关等原因来不及恢复，导致整个游戏世界永久慢动作
	if (UGameInstance* GI = GetGameInstance())
	{
		UGameplayStatics::SetGlobalTimeDilation(GI, 1.0f);
	}

	// 清掉 ticker，防止野指针回调
	if (DilationTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DilationTickerHandle);
		DilationTickerHandle.Reset();
	}

	Super::Deinitialize();
}

void UTutorialManager::Init(UTutorialPopupWidget* InWidget, UTutorialRegistryDA* InRegistry)
{
	PopupWidget = InWidget;
	Registry    = InRegistry;

	UE_LOG(LogTemp, Log, TEXT("[Tutorial] Init: PopupWidget=%s, Registry=%s"),
		InWidget    ? *InWidget->GetClass()->GetName()  : TEXT("NULL — TutorialPopupClass 未在 BP_HUD 设置!"),
		InRegistry  ? *InRegistry->GetName()            : TEXT("NULL — TutorialRegistry 未在 BP_HUD 设置!"));
}

void UTutorialManager::LoadFromSave(UYogSaveGame* Save)
{
	if (!Save) return;
	State = Save->TutorialState;
}

// ============================================================
//  阶段顺序判断（不依赖 enum 数值大小）
// ============================================================

int32 UTutorialManager::StageRank(ETutorialState S) const
{
	switch (S)
	{
	case ETutorialState::None:                   return -1;  // 未初始化 / 禁用
	case ETutorialState::NeedWeaponTutorial:     return 0;
	case ETutorialState::WeaponTutorialDone:     return 1;
	case ETutorialState::NeedPostCombatTutorial: return 1;   // 旧/废弃，与 WeaponTutorialDone 同级
	case ETutorialState::NeedFirstRuneTutorial:  return 2;
	case ETutorialState::NeedBackpackTutorial:   return 3;
	case ETutorialState::NeedHeatPhaseTutorial:  return 4;
	case ETutorialState::Completed:              return 5;
	default:                                     return -2;  // 未识别状态
	}
}

bool UTutorialManager::HasPassedStage(ETutorialState Required) const
{
	const int32 Cur = StageRank(State);
	const int32 Req = StageRank(Required);
	return Cur >= 0 && Req >= 0 && Cur > Req;
}

// ============================================================
//  旧两段流（保留兼容；状态推进改到弹窗显示之后）
// ============================================================

void UTutorialManager::TryWeaponTutorial(AYogPlayerControllerBase* PC)
{
	UE_LOG(LogTemp, Log, TEXT("[Tutorial] TryWeaponTutorial: State=%d, PopupValid=%d, PC=%s"),
		(int32)State, PopupWidget.IsValid() ? 1 : 0, PC ? *PC->GetName() : TEXT("null"));

	if (State != ETutorialState::NeedWeaponTutorial) return;
	if (!PopupWidget.IsValid() || !PC) return;
	if (bPopupShowing) return;
	// 防 0.35s 窗口期重入：状态推进延后到 DoShowWeaponPopup，期间多次调用会通过 State guard
	if (DilationTickerHandle.IsValid()) return;

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	// 1. 时间膨胀降速，营造"世界停顿"氛围
	UGameplayStatics::SetGlobalTimeDilation(GI, 0.08f);

	// 2. FTSTicker 基于真实时间计时（不受 TimeDilation 影响），0.35s 后显示弹窗
	TWeakObjectPtr<AYogPlayerControllerBase> WeakPC = PC;
	TWeakObjectPtr<UTutorialManager> WeakThis = this;
	TWeakObjectPtr<UGameInstance> WeakGI = GI;

	DilationTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakThis, WeakPC, WeakGI](float) -> bool
		{
			// 兜底：无论 WeakThis 是否还在，都先恢复全局时间膨胀
			if (WeakGI.IsValid())
			{
				UGameplayStatics::SetGlobalTimeDilation(WeakGI.Get(), 1.0f);
			}
			if (WeakThis.IsValid())
			{
				WeakThis->DilationTickerHandle.Reset();
				WeakThis->DoShowWeaponPopup(WeakPC);
			}
			return false; // 只触发一次
		}),
		0.35f);
}

void UTutorialManager::TryPostCombatTutorial(AYogPlayerControllerBase* PC)
{
	if (State != ETutorialState::WeaponTutorialDone) return;
	if (!PopupWidget.IsValid() || !PC) return;
	if (bPopupShowing) return;

	UGameInstance* GI = GetGameInstance();
	UWorld* World = GI ? GI->GetWorld() : nullptr;
	if (!World) return;

	State = ETutorialState::NeedPostCombatTutorial;

	TWeakObjectPtr<AYogPlayerControllerBase> WeakPC = PC;
	TWeakObjectPtr<UTutorialManager> WeakThis = this;

	World->GetTimerManager().SetTimer(
		DelayHandle,
		[WeakThis, WeakPC]()
		{
			if (WeakThis.IsValid()) WeakThis->DoShowPostCombatPopup(WeakPC);
		},
		0.2f, false);
}

void UTutorialManager::DoShowWeaponPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC)
{
	UE_LOG(LogTemp, Log, TEXT("[Tutorial] DoShowWeaponPopup fired"));
	if (!WeakPC.IsValid() || !PopupWidget.IsValid()) return;

	// 选取页面（注册表 → 兜底文本）
	TArray<FTutorialPage> PagesToShow;
	if (Registry)
	{
		if (const TArray<FTutorialPage>* RegisteredPages = Registry->FindPages(EventID_WeaponTutorial))
		{
			PagesToShow = *RegisteredPages;
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[Tutorial] Registry 中未找到 EventID='WeaponTutorial'，使用兜底文字"));
		}
	}
	if (PagesToShow.Num() == 0)
	{
		PagesToShow.Add({ LOCTEXT("WeaponTitle", "配置你的符文"),
		                  LOCTEXT("WeaponBody", "激活区内的符文才会在战斗中生效。\n你可以拖动符文调整位置。") });
	}

	// ShowPopup 真正调用之后才推进 + 持久化
	bPopupShowing = true;
	PopupWidget->ShowPopup(PagesToShow);

	State = ETutorialState::WeaponTutorialDone;
	SaveState();
}

void UTutorialManager::DoShowPostCombatPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC)
{
	UE_LOG(LogTemp, Log, TEXT("[Tutorial] DoShowPostCombatPopup fired"));
	if (!WeakPC.IsValid() || !PopupWidget.IsValid()) return;

	// 先 OpenBackpack（这是教程的一部分），再 ShowPopup
	WeakPC->OpenBackpack();

	TArray<FTutorialPage> PagesToShow;
	if (Registry)
	{
		if (const TArray<FTutorialPage>* RegisteredPages = Registry->FindPages(EventID_PostCombatTutorial))
		{
			PagesToShow = *RegisteredPages;
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[Tutorial] Registry 中未找到 EventID='PostCombatTutorial'，使用兜底文字"));
		}
	}
	if (PagesToShow.Num() == 0)
	{
		PagesToShow.Add({ LOCTEXT("PostCombatTitle", "放置你的新符文"),
		                  LOCTEXT("PostCombatBody", "把新符文移动到你想要的位置。\n进入下一关后，战斗中将无法调整。") });
	}

	bPopupShowing = true;
	PopupWidget->ShowPopup(PagesToShow);

	// 旧路径推进到 NeedFirstRuneTutorial（让新四段流接管），不再直接 Completed
	// 这样旧调用者也能逐步过渡到新流程
	State = ETutorialState::NeedFirstRuneTutorial;
	SaveState();
}

// =========================================================
// 新四段教程流 ②③④（① 武器拾取走 LevelFlow + LENode_ShowTutorial）
// 状态推进策略：仅在 ShowByEventID 实际显示弹窗后才 SaveState，且使用 StageRank 判断阶段
// =========================================================

void UTutorialManager::TryFirstRuneTutorial(APlayerController* PC)
{
	if (State == ETutorialState::None) return;                     // 未初始化拦
	if (HasPassedStage(ETutorialState::NeedFirstRuneTutorial)) return;  // 已通过此阶段
	if (!PopupWidget.IsValid() || !PC) return;

	// slow-mo 期间触发，不强暂停（避免打断关卡结束运镜）
	if (ShowByEventID(EventID_FirstRune, PC, /*bPauseGame=*/false))
	{
		State = ETutorialState::NeedBackpackTutorial;
		SaveState();
	}
}

void UTutorialManager::TryBackpackTutorial(APlayerController* PC)
{
	if (State == ETutorialState::None) return;
	if (HasPassedStage(ETutorialState::NeedBackpackTutorial)) return;
	if (!PopupWidget.IsValid() || !PC) return;

	// 玩家在看背包 UI（已是暂停状态），bPauseGame 影响不大；保持 true 与 UI 暂停一致
	if (ShowByEventID(EventID_Backpack, PC, /*bPauseGame=*/true))
	{
		State = ETutorialState::NeedHeatPhaseTutorial;
		SaveState();
	}
}

void UTutorialManager::TryHeatPhaseTutorial(APlayerController* PC)
{
	if (State == ETutorialState::None) return;
	if (HasPassedStage(ETutorialState::NeedHeatPhaseTutorial)) return;
	if (!PopupWidget.IsValid() || !PC) return;

	// 战斗中触发，不暂停（信息浮窗）
	if (ShowByEventID(EventID_ActivationZone, PC, /*bPauseGame=*/false))
	{
		State = ETutorialState::Completed;
		SaveState();
	}
}

bool UTutorialManager::ShowInlinePages(const TArray<FTutorialPage>& Pages, APlayerController* /*PC*/, bool bPauseGame)
{
	if (!PopupWidget.IsValid() || Pages.IsEmpty())
	{
		// 防 LevelFlow 节点 hang：失败时也广播一次，让等待 OnPopupClosed 的节点立即继续
		// 已知副作用：若期间还有别的弹窗的等待者，会被一并触发 — 重复调用本身就罕见，可接受
		OnPopupClosed.Broadcast();
		return false;
	}
	if (bPopupShowing)
	{
		UE_LOG(LogTemp, Log, TEXT("[Tutorial] ShowInlinePages 被忽略：当前已有教程弹窗"));
		OnPopupClosed.Broadcast();
		return false;
	}

	bPopupShowing = true;
	PopupWidget->ShowPopup(Pages, bPauseGame);
	return true;
}

bool UTutorialManager::ShowByEventID(FName EventID, APlayerController* /*PC*/, bool bPauseGame)
{
	if (!PopupWidget.IsValid())
	{
		OnPopupClosed.Broadcast();  // 防 LevelFlow 节点 hang
		return false;
	}
	if (bPopupShowing)
	{
		UE_LOG(LogTemp, Log, TEXT("[Tutorial] ShowByEventID(%s) 被忽略：当前已有教程弹窗"), *EventID.ToString());
		OnPopupClosed.Broadcast();  // 防 LevelFlow 节点 hang
		return false;
	}

	TArray<FTutorialPage> PagesToShow;
	if (Registry)
	{
		if (const TArray<FTutorialPage>* RegisteredPages = Registry->FindPages(EventID))
		{
			PagesToShow = *RegisteredPages;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Tutorial] Registry 中未找到 EventID='%s'，使用兜底文字"), *EventID.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Tutorial] Registry 未配置，EventID='%s' 使用兜底文字"), *EventID.ToString());
	}

	// 兜底：单页显示 EventID 本身
	if (PagesToShow.Num() == 0)
	{
		PagesToShow.Add({ FText::FromName(EventID), FText::GetEmpty() });
	}

	bPopupShowing = true;
	PopupWidget->ShowPopup(PagesToShow, bPauseGame);
	return true;
}

void UTutorialManager::NotifyPopupClosed()
{
	bPopupShowing = false;

	// 注意：不在此处恢复 GlobalTimeDilation。
	// - 武器教程 ticker 内部已无条件恢复（即使 WeakThis 失效）
	// - Subsystem Deinitialize 是最后兜底
	// - FirstRune 等教程在关卡结束 slow-mo 期间触发，关闭弹窗时 slow-mo 应继续，由 HUD::TickLevelEndEffect 控制
	OnPopupClosed.Broadcast();
}

void UTutorialManager::ForceClosePopup()
{
	if (PopupWidget.IsValid() && bPopupShowing)
	{
		PopupWidget->ConfirmClose();
	}
}

void UTutorialManager::SaveState()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UYogSaveSubsystem* SaveSys = GI->GetSubsystem<UYogSaveSubsystem>();
	if (!SaveSys) return;

	UYogSaveGame* Save = SaveSys->GetCurrentSave();
	if (!Save) return;

	Save->TutorialState = State;
	SaveSys->WriteSaveGame();
}

#undef LOCTEXT_NAMESPACE
