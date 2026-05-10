#include "Tutorial/TutorialManager.h"

#include "Character/YogPlayerControllerBase.h"
#include "Containers/Ticker.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "UI/GameDialogWidget.h"
#include "UI/TutorialRegistryDA.h"
#include "Visual/TimeDilationVisualSubsystem.h"

namespace
{
	FName TutorialEventID(const TCHAR* EventID)
	{
		return FName(EventID);
	}

	FTutorialPage MakeTutorialPage(const TCHAR* Title, const TCHAR* Body, const TCHAR* SubText = TEXT(""))
	{
		FTutorialPage Page;
		Page.Title = FText::FromString(Title);
		Page.Body = FText::FromString(Body);
		Page.SubText = FText::FromString(SubText);
		return Page;
	}
}

void UTutorialManager::Deinitialize()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		UGameplayStatics::SetGlobalTimeDilation(GI, 1.0f);
	}

	if (DilationTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DilationTickerHandle);
		DilationTickerHandle.Reset();
	}
	EndDilationVisualIfActive();

	Super::Deinitialize();
}

void UTutorialManager::Init(UTutorialPopupWidget* InWidget, UTutorialRegistryDA* InRegistry)
{
	PopupWidget = InWidget;
	Registry = InRegistry;

	UE_LOG(LogTemp, Log, TEXT("[Tutorial] Init: PopupWidget=%s, Registry=%s"),
		InWidget ? *InWidget->GetClass()->GetName() : TEXT("NULL - TutorialPopupClass is not set on BP_HUD"),
		InRegistry ? *InRegistry->GetName() : TEXT("NULL - TutorialRegistry is not set on BP_HUD"));
}

void UTutorialManager::LoadFromSave(UYogSaveGame* Save)
{
	if (!Save)
	{
		return;
	}

	State = Save->TutorialState;
}

int32 UTutorialManager::StageRank(ETutorialState S) const
{
	switch (S)
	{
	case ETutorialState::None:                   return -1;
	case ETutorialState::NeedWeaponTutorial:     return 0;
	case ETutorialState::WeaponTutorialDone:     return 1;
	case ETutorialState::NeedPostCombatTutorial: return 1; // Legacy alias for the post-loot reward-card step.
	case ETutorialState::NeedFirstRuneTutorial:  return 2; // 512: first reward card has entered the combat deck.
	case ETutorialState::NeedBackpackTutorial:   return 3; // 512: first deck arrangement tutorial.
	case ETutorialState::NeedHeatPhaseTutorial:  return 4; // 512: first Link-card tutorial.
	case ETutorialState::Completed:              return 5;
	default:                                     return -2;
	}
}

bool UTutorialManager::HasPassedStage(ETutorialState Required) const
{
	const int32 Cur = StageRank(State);
	const int32 Req = StageRank(Required);
	return Cur >= 0 && Req >= 0 && Cur > Req;
}

void UTutorialManager::TryWeaponTutorial(AYogPlayerControllerBase* PC)
{
	UE_LOG(LogTemp, Log, TEXT("[Tutorial] TryWeaponTutorial: State=%d, PopupValid=%d, PC=%s"),
		(int32)State, PopupWidget.IsValid() ? 1 : 0, PC ? *PC->GetName() : TEXT("null"));

	if (State != ETutorialState::NeedWeaponTutorial)
	{
		return;
	}
	if (!PopupWidget.IsValid() || !PC || bPopupShowing || DilationTickerHandle.IsValid())
	{
		return;
	}

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(GI, 0.08f);
	UTimeDilationVisualSubsystem::BeginTimeDilationVisual(GI);
	bDilationVisualActive = true;

	TWeakObjectPtr<AYogPlayerControllerBase> WeakPC = PC;
	TWeakObjectPtr<UTutorialManager> WeakThis = this;
	TWeakObjectPtr<UGameInstance> WeakGI = GI;

	DilationTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([WeakThis, WeakPC, WeakGI](float) -> bool
		{
			if (WeakGI.IsValid())
			{
				UGameplayStatics::SetGlobalTimeDilation(WeakGI.Get(), 1.0f);
			}
			if (WeakThis.IsValid())
			{
				WeakThis->DilationTickerHandle.Reset();
				WeakThis->EndDilationVisualIfActive();
				WeakThis->DoShowWeaponPopup(WeakPC);
			}
			return false;
		}),
		0.35f);
}

void UTutorialManager::TryPostCombatTutorial(AYogPlayerControllerBase* PC)
{
	if (State == ETutorialState::None)
	{
		return;
	}
	if (State == ETutorialState::NeedWeaponTutorial || HasPassedStage(ETutorialState::NeedFirstRuneTutorial))
	{
		return;
	}
	if (!PopupWidget.IsValid() || !PC || bPopupShowing)
	{
		return;
	}

	// 512 keeps this old function name as the loot-selection compatibility entry.
	// It no longer opens the old backpack grid; it only explains that the reward card entered the deck.
	if (ShowByEventID(TutorialEventID(TEXT("tutorial_first_rune")), PC, /*bPauseGame=*/true))
	{
		State = ETutorialState::NeedBackpackTutorial;
		SaveState();
	}
}

void UTutorialManager::DoShowWeaponPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC)
{
	UE_LOG(LogTemp, Log, TEXT("[Tutorial] DoShowWeaponPopup fired"));
	if (!WeakPC.IsValid() || !PopupWidget.IsValid())
	{
		return;
	}

	TArray<FTutorialPage> PagesToShow;
	if (Registry)
	{
		if (const TArray<FTutorialPage>* RegisteredPages = Registry->FindPages(TutorialEventID(TEXT("tutorial_weapon_pickup"))))
		{
			PagesToShow = *RegisteredPages;
		}
		else if (const TArray<FTutorialPage>* LegacyPages = Registry->FindPages(TutorialEventID(TEXT("WeaponTutorial"))))
		{
			PagesToShow = *LegacyPages;
		}
	}

	if (PagesToShow.Num() == 0)
	{
		PagesToShow.Add(MakeTutorialPage(
			TEXT("拾起武器"),
			TEXT("靠近发光武器，按 [E] 拾取。武器会装备到角色身上，并把它自带的初始卡牌装入下方 1D 卡组。")));
		PagesToShow.Add(MakeTutorialPage(
			TEXT("武器自带初始卡牌"),
			TEXT("每把武器都有一组起始卡。拾取后，卡组会按武器配置顺序显示；轻/重攻击不会被卡组阻止，但命中会按顺序消耗卡牌并触发效果。")));
	}

	bPopupShowing = true;
	PopupWidget->ShowPopup(PagesToShow);

	State = ETutorialState::WeaponTutorialDone;
	SaveState();
}

void UTutorialManager::DoShowPostCombatPopup(TWeakObjectPtr<AYogPlayerControllerBase> WeakPC)
{
	UE_LOG(LogTemp, Log, TEXT("[Tutorial] DoShowPostCombatPopup fired through legacy path"));
	if (!WeakPC.IsValid())
	{
		return;
	}

	TryPostCombatTutorial(WeakPC.Get());
}

void UTutorialManager::TryFirstRuneTutorial(APlayerController* PC)
{
	if (State == ETutorialState::None)
	{
		return;
	}
	if (State == ETutorialState::NeedWeaponTutorial || HasPassedStage(ETutorialState::NeedFirstRuneTutorial))
	{
		return;
	}
	if (!PopupWidget.IsValid() || !PC || bPopupShowing)
	{
		return;
	}

	if (ShowByEventID(TutorialEventID(TEXT("tutorial_first_rune")), PC, /*bPauseGame=*/true))
	{
		State = ETutorialState::NeedBackpackTutorial;
		SaveState();
	}
}

void UTutorialManager::TryBackpackTutorial(APlayerController* PC)
{
	if (State == ETutorialState::NeedHeatPhaseTutorial)
	{
		TryCardLinkTutorial(PC);
		return;
	}

	if (State == ETutorialState::None)
	{
		return;
	}
	if (HasPassedStage(ETutorialState::NeedBackpackTutorial))
	{
		return;
	}
	if (!PopupWidget.IsValid() || !PC || bPopupShowing)
	{
		return;
	}

	if (ShowByEventID(TutorialEventID(TEXT("tutorial_backpack")), PC, /*bPauseGame=*/true))
	{
		State = ETutorialState::NeedHeatPhaseTutorial;
		SaveState();
	}
}

void UTutorialManager::TryHeatPhaseTutorial(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	if (UGameplayStatics::IsGamePaused(PC))
	{
		TryCardLinkTutorial(PC);
		return;
	}

	UE_LOG(LogTemp, Verbose, TEXT("[Tutorial] Deprecated heat tutorial skipped outside a safe paused UI state"));
}

void UTutorialManager::TryCardLinkTutorial(APlayerController* PC)
{
	if (State == ETutorialState::None)
	{
		return;
	}
	if (StageRank(State) < StageRank(ETutorialState::NeedHeatPhaseTutorial))
	{
		return;
	}
	if (HasPassedStage(ETutorialState::NeedHeatPhaseTutorial))
	{
		return;
	}
	if (!PopupWidget.IsValid() || !PC || bPopupShowing)
	{
		return;
	}

	if (ShowByEventID(TutorialEventID(TEXT("tutorial_card_link")), PC, /*bPauseGame=*/true))
	{
		State = ETutorialState::Completed;
		SaveState();
	}
}

bool UTutorialManager::ShowInlinePages(const TArray<FTutorialPage>& Pages, APlayerController* /*PC*/, bool bPauseGame)
{
	if (!PopupWidget.IsValid() || Pages.IsEmpty())
	{
		OnPopupClosed.Broadcast();
		return false;
	}
	if (bPopupShowing)
	{
		UE_LOG(LogTemp, Log, TEXT("[Tutorial] ShowInlinePages ignored because a tutorial popup is already showing"));
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
		OnPopupClosed.Broadcast();
		return false;
	}
	if (bPopupShowing)
	{
		UE_LOG(LogTemp, Log, TEXT("[Tutorial] ShowByEventID(%s) ignored because a tutorial popup is already showing"), *EventID.ToString());
		OnPopupClosed.Broadcast();
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
			UE_LOG(LogTemp, Warning, TEXT("[Tutorial] Registry does not contain EventID='%s'; using fallback text"), *EventID.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Tutorial] Registry is not configured; EventID='%s' uses fallback text"), *EventID.ToString());
	}

	if (PagesToShow.Num() == 0)
	{
		PagesToShow.Add(MakeTutorialPage(*EventID.ToString(), TEXT("")));
	}

	bPopupShowing = true;
	PopupWidget->ShowPopup(PagesToShow, bPauseGame);
	return true;
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

void UTutorialManager::EndDilationVisualIfActive()
{
	if (!bDilationVisualActive)
	{
		return;
	}

	UTimeDilationVisualSubsystem::EndTimeDilationVisual(GetGameInstance());
	bDilationVisualActive = false;
}

void UTutorialManager::SaveState()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	UYogSaveSubsystem* SaveSys = GI->GetSubsystem<UYogSaveSubsystem>();
	if (!SaveSys)
	{
		return;
	}

	UYogSaveGame* Save = SaveSys->GetCurrentSave();
	if (!Save)
	{
		return;
	}

	Save->TutorialState = State;
	SaveSys->WriteSaveGame();
}
