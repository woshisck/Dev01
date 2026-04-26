#include "Tutorial/TutorialManager.h"
#include "UI/GameDialogWidget.h"
#include "UI/TutorialRegistryDA.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "Character/YogPlayerControllerBase.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/Ticker.h"

#define LOCTEXT_NAMESPACE "Tutorial"

// DA 里的 EventID 键名
static const FName EventID_WeaponTutorial    = "WeaponTutorial";
static const FName EventID_PostCombatTutorial = "PostCombatTutorial";

// 新四段教程流的 EventID（与 UTutorialRegistryDA::Entries 的 Key 一字不差）
static const FName EventID_WeaponPickup    = "tutorial_weapon_pickup";
static const FName EventID_FirstRune       = "tutorial_first_rune";
static const FName EventID_Backpack        = "tutorial_backpack";
static const FName EventID_ActivationZone  = "tutorial_activation_zone";

void UTutorialManager::Init(UTutorialPopupWidget* InWidget, UTutorialRegistryDA* InRegistry)
{
	PopupWidget = InWidget;
	Registry    = InRegistry;

	UE_LOG(LogTemp, Warning, TEXT("[Tutorial] Init: PopupWidget=%s, Registry=%s"),
		InWidget    ? *InWidget->GetClass()->GetName()  : TEXT("NULL — TutorialPopupClass 未在 BP_HUD 设置!"),
		InRegistry  ? *InRegistry->GetName()            : TEXT("NULL — TutorialRegistry 未在 BP_HUD 设置!"));
}

void UTutorialManager::LoadFromSave(UYogSaveGame* Save)
{
	if (!Save) return;
	State = Save->TutorialState;
}

// IsPopupShowing() 是内联的，由 bPopupShowing 驱动，不依赖 Widget 状态

void UTutorialManager::TryWeaponTutorial(AYogPlayerControllerBase* PC)
{
	UE_LOG(LogTemp, Warning, TEXT("[Tutorial] TryWeaponTutorial: State=%d, PopupValid=%d, PC=%s"),
		(int32)State, PopupWidget.IsValid() ? 1 : 0, PC ? *PC->GetName() : TEXT("null"));

	if (State != ETutorialState::NeedWeaponTutorial) return;
	if (!PopupWidget.IsValid() || !PC) return;

	State = ETutorialState::WeaponTutorialDone;
	SaveState();

	// 1. 时间膨胀降速，营造"世界停顿"氛围
	UGameplayStatics::SetGlobalTimeDilation(GetGameInstance(), 0.08f);

	// 2. FTSTicker 基于真实时间计时（不受 TimeDilation 影响），0.35s 后显示弹窗
	TWeakObjectPtr<AYogPlayerControllerBase> WeakPC = PC;
	TWeakObjectPtr<UTutorialManager> WeakThis = this;

	DilationTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakThis, WeakPC](float) -> bool
		{
			if (WeakThis.IsValid())
			{
				// 恢复正常时间后显示弹窗（弹窗内部会调用 SetGamePaused）
				UGameplayStatics::SetGlobalTimeDilation(WeakThis->GetGameInstance(), 1.0f);
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

	State = ETutorialState::NeedPostCombatTutorial;

	TWeakObjectPtr<AYogPlayerControllerBase> WeakPC = PC;
	TWeakObjectPtr<UTutorialManager> WeakThis = this;

	GetGameInstance()->GetWorld()->GetTimerManager().SetTimer(
		DelayHandle,
		[WeakThis, WeakPC]()
		{
			if (WeakThis.IsValid()) WeakThis->DoShowPostCombatPopup(WeakPC);
		},
		0.2f, false);
}

void UTutorialManager::DoShowWeaponPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC)
{
	UE_LOG(LogTemp, Warning, TEXT("[Tutorial] DoShowWeaponPopup fired"));
	if (!WeakPC.IsValid() || !PopupWidget.IsValid()) return;

	bPopupShowing = true;

	// 优先从注册表读页面；未配置则用兜底文字
	if (Registry)
	{
		if (const TArray<FTutorialPage>* Pages = Registry->FindPages(EventID_WeaponTutorial))
		{
			PopupWidget->ShowPopup(*Pages);
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("[Tutorial] Registry 中未找到 EventID='WeaponTutorial'，使用兜底文字"));
	}

	TArray<FTutorialPage> Pages;
	Pages.Add({ LOCTEXT("WeaponTitle", "配置你的符文"),
	            LOCTEXT("WeaponBody", "激活区内的符文才会在战斗中生效。\n你可以拖动符文调整位置。") });
	PopupWidget->ShowPopup(Pages);
}

void UTutorialManager::DoShowPostCombatPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC)
{
	UE_LOG(LogTemp, Warning, TEXT("[Tutorial] DoShowPostCombatPopup fired"));
	if (!WeakPC.IsValid() || !PopupWidget.IsValid()) return;

	bPopupShowing = true;

	State = ETutorialState::Completed;
	SaveState();

	WeakPC->OpenBackpack();

	if (Registry)
	{
		if (const TArray<FTutorialPage>* Pages = Registry->FindPages(EventID_PostCombatTutorial))
		{
			PopupWidget->ShowPopup(*Pages);
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("[Tutorial] Registry 中未找到 EventID='PostCombatTutorial'，使用兜底文字"));
	}

	TArray<FTutorialPage> Pages;
	Pages.Add({ LOCTEXT("PostCombatTitle", "放置你的新符文"),
	            LOCTEXT("PostCombatBody", "把新符文移动到你想要的位置。\n进入下一关后，战斗中将无法调整。") });
	PopupWidget->ShowPopup(Pages);
}

// =========================================================
// 新四段教程流 ②③④（① 走 LevelFlow + LENode_ShowTutorial）
// State guard: 用 `> 当前阶段` 判断是否已通过此阶段，向后单调推进
// =========================================================

void UTutorialManager::TryFirstRuneTutorial(APlayerController* PC)
{
	if (State > ETutorialState::NeedFirstRuneTutorial) return; // 已通过此阶段
	if (!PopupWidget.IsValid() || !PC) return;

	State = ETutorialState::NeedBackpackTutorial;
	SaveState();

	// slow-mo 期间触发，不强暂停（避免打断关卡结束运镜）
	ShowByEventID(EventID_FirstRune, PC, /*bPauseGame=*/false);
}

void UTutorialManager::TryBackpackTutorial(APlayerController* PC)
{
	if (State > ETutorialState::NeedBackpackTutorial) return;
	if (!PopupWidget.IsValid() || !PC) return;

	State = ETutorialState::NeedHeatPhaseTutorial;
	SaveState();

	// 玩家在看背包 UI（已是暂停状态），bPauseGame 影响不大；保持 true 与 UI 暂停一致
	ShowByEventID(EventID_Backpack, PC, /*bPauseGame=*/true);
}

void UTutorialManager::TryHeatPhaseTutorial(APlayerController* PC)
{
	if (State > ETutorialState::NeedHeatPhaseTutorial) return;
	if (!PopupWidget.IsValid() || !PC) return;

	State = ETutorialState::Completed;
	SaveState();

	// 战斗中触发，不暂停（信息浮窗）
	ShowByEventID(EventID_ActivationZone, PC, /*bPauseGame=*/false);
}

void UTutorialManager::ShowInlinePages(const TArray<FTutorialPage>& Pages, APlayerController* PC, bool bPauseGame)
{
	if (!PopupWidget.IsValid() || Pages.IsEmpty()) return;
	bPopupShowing = true;
	PopupWidget->ShowPopup(Pages, bPauseGame);
}

void UTutorialManager::ShowByEventID(FName EventID, APlayerController* PC, bool bPauseGame)
{
	if (!PopupWidget.IsValid()) return;

	bPopupShowing = true;

	if (Registry)
	{
		if (const TArray<FTutorialPage>* Pages = Registry->FindPages(EventID))
		{
			PopupWidget->ShowPopup(*Pages, bPauseGame);
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("[Tutorial] Registry 中未找到 EventID='%s'，使用兜底文字"), *EventID.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Tutorial] Registry 未配置，EventID='%s' 使用兜底文字"), *EventID.ToString());
	}

	// 兜底：单页显示 EventID 本身
	TArray<FTutorialPage> Pages;
	Pages.Add({ FText::FromName(EventID), FText::GetEmpty() });
	PopupWidget->ShowPopup(Pages, bPauseGame);
}

void UTutorialManager::NotifyPopupClosed()
{
	bPopupShowing = false;
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
	UYogSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UYogSaveSubsystem>();
	if (!SaveSys) return;

	UYogSaveGame* Save = SaveSys->GetCurrentSave();
	if (!Save) return;

	Save->TutorialState = State;
	SaveSys->WriteSaveGame();
}

#undef LOCTEXT_NAMESPACE
