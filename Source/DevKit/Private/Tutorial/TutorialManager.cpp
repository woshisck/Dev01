#include "Tutorial/TutorialManager.h"
#include "UI/GameDialogWidget.h"
#include "UI/DialogContentDA.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "Character/YogPlayerControllerBase.h"

#define LOCTEXT_NAMESPACE "Tutorial"

// DA 里的 EventID 键名
static const FName EventID_WeaponTutorial    = "WeaponTutorial";
static const FName EventID_PostCombatTutorial = "PostCombatTutorial";

void UTutorialManager::Init(UTutorialPopupWidget* InWidget, UDialogContentDA* InContentDA)
{
	PopupWidget = InWidget;
	ContentDA   = InContentDA;

	UE_LOG(LogTemp, Warning, TEXT("[Tutorial] Init: PopupWidget=%s, ContentDA=%s"),
		InWidget    ? *InWidget->GetClass()->GetName()  : TEXT("NULL — TutorialPopupClass 未在 BP_HUD 设置!"),
		InContentDA ? *InContentDA->GetName()           : TEXT("NULL — DialogContentDA 未在 BP_HUD 设置!"));
}

void UTutorialManager::LoadFromSave(UYogSaveGame* Save)
{
	if (!Save) return;
	State = Save->TutorialState;
}

void UTutorialManager::TryWeaponTutorial(AYogPlayerControllerBase* PC)
{
	UE_LOG(LogTemp, Warning, TEXT("[Tutorial] TryWeaponTutorial: State=%d, PopupValid=%d, PC=%s"),
		(int32)State, PopupWidget.IsValid() ? 1 : 0, PC ? *PC->GetName() : TEXT("null"));

	if (State != ETutorialState::NeedWeaponTutorial) return;
	if (!PopupWidget.IsValid() || !PC) return;

	State = ETutorialState::WeaponTutorialDone;
	SaveState();

	TWeakObjectPtr<AYogPlayerControllerBase> WeakPC = PC;
	TWeakObjectPtr<UTutorialManager> WeakThis = this;

	GetGameInstance()->GetWorld()->GetTimerManager().SetTimer(
		DelayHandle,
		[WeakThis, WeakPC]()
		{
			if (WeakThis.IsValid()) WeakThis->DoShowWeaponPopup(WeakPC);
		},
		0.4f, false);
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

	WeakPC->OpenBackpack();

	// 优先从 DA 读页面；DA 未配置则用兜底文字
	if (ContentDA)
	{
		if (const TArray<FTutorialPage>* Pages = ContentDA->FindPages(EventID_WeaponTutorial))
		{
			PopupWidget->ShowPopup(*Pages);
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("[Tutorial] DA 中未找到 EventID='WeaponTutorial'，使用兜底文字"));
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

	State = ETutorialState::Completed;
	SaveState();

	WeakPC->OpenBackpack();

	if (ContentDA)
	{
		if (const TArray<FTutorialPage>* Pages = ContentDA->FindPages(EventID_PostCombatTutorial))
		{
			PopupWidget->ShowPopup(*Pages);
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("[Tutorial] DA 中未找到 EventID='PostCombatTutorial'，使用兜底文字"));
	}

	TArray<FTutorialPage> Pages;
	Pages.Add({ LOCTEXT("PostCombatTitle", "放置你的新符文"),
	            LOCTEXT("PostCombatBody", "把新符文移动到你想要的位置。\n进入下一关后，战斗中将无法调整。") });
	PopupWidget->ShowPopup(Pages);
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
