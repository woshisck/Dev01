#include "Tutorial/TutorialManager.h"

#include "Character/YogPlayerControllerBase.h"
#include "Containers/Ticker.h"
#include "Data/LevelInfoPopupDA.h"
#include "Engine/LocalPlayer.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "UI/GameDialogWidget.h"
#include "UI/TutorialRegistryDA.h"
#include "UI/YogHUD.h"
#include "UI/YogUIManagerSubsystem.h"
#include "Visual/TimeDilationVisualSubsystem.h"

namespace
{
	FName TutorialEventID(const TCHAR* EventID)
	{
		return FName(EventID);
	}

	const FName LinkCardTutorialEventID(TEXT("tutorial_card_link"));
	const FName MoonlightLinkCardTutorialEventID(TEXT("tutorial_card_link_moonlight"));
	const FName WeaponPickupTutorialEventID(TEXT("tutorial_weapon_pickup"));
	const TCHAR* DefaultTutorialRegistryObjectPath =
		TEXT("/Game/Docs/UI/Tutorial/DA_TutorialRegistry.DA_TutorialRegistry");

	FGameplayTag GetLinkCardHintTag()
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Tutorial.Hint.LinkCard"), false);
	}

	bool HasRegisteredTutorialPages(const UTutorialRegistryDA* InRegistry, FName EventID)
	{
		if (!InRegistry)
		{
			return false;
		}

		const TArray<FTutorialPage>* Pages = InRegistry->FindPages(EventID);
		return Pages && !Pages->IsEmpty();
	}

	FName ResolveLinkCardTutorialEventIdFromRegistry(const UTutorialRegistryDA* InRegistry)
	{
		return HasRegisteredTutorialPages(InRegistry, MoonlightLinkCardTutorialEventID)
			? MoonlightLinkCardTutorialEventID
			: LinkCardTutorialEventID;
	}

	FTutorialPage MakeTutorialPage(const TCHAR* Title, const TCHAR* Body, const TCHAR* SubText = TEXT(""))
	{
		FTutorialPage Page;
		Page.Title = FText::FromString(Title);
		Page.Body = FText::FromString(Body);
		Page.SubText = FText::FromString(SubText);
		return Page;
	}

	void BuildFallbackPagesForEventID(FName EventID, TArray<FTutorialPage>& OutPages)
	{
		if (EventID == FName(TEXT("tutorial_moonlight_pickup")))
		{
			OutPages.Add(MakeTutorialPage(
				TEXT("获得月光·连携卡"),
				TEXT("月光是一张特殊的连携卡，自身可造成斩击伤害，同时能与卡组中相邻的卡牌产生联动效果。打开背包调整卡牌顺序，以决定连携的方向。")));
			OutPages.Add(MakeTutorialPage(
				TEXT("正向连携"),
				TEXT("将月光放在普通卡或重击卡的右侧（后方）：前方的卡牌命中后，月光接续触发正向连携，追加额外效果。")));
			OutPages.Add(MakeTutorialPage(
				TEXT("反向连携"),
				TEXT("将月光放在普通卡或重击卡的左侧（前方）：月光命中后，触发反向连携，为后方卡牌蓄积特殊增益。")));
		}
		else if (EventID == FName(TEXT("tutorial_backpack")))
		{
			OutPages.Add(MakeTutorialPage(
				TEXT("卡牌背包"),
				TEXT("战斗卡牌会依次消耗。打开背包后，可以拖拽调整卡牌顺序，让连击节奏更流畅。")));
		}
		else if (EventID == FName(TEXT("tutorial_first_rune")))
		{
			OutPages.Add(MakeTutorialPage(
				TEXT("获得战斗卡"),
				TEXT("新卡已装入卡组。打开背包可以查看当前卡牌，并调整它们的战斗顺序。")));
		}
		else if (EventID == FName(TEXT("tutorial_card_link")) || EventID == FName(TEXT("tutorial_card_link_moonlight")))
		{
			OutPages.Add(MakeTutorialPage(
				TEXT("连携卡"),
				TEXT("连携卡放在相邻位置时会与旁边的卡牌产生联动。正向连携（卡牌后接月光）或反向连携（月光后接卡牌）均可触发，效果各有不同。")));
		}
	}

	UYogUIManagerSubsystem* PrepareTutorialPopupManager(UTutorialPopupWidget* FallbackWidget, APlayerController* PC, bool bPauseGame)
	{
		if (!PC)
		{
			return nullptr;
		}

		ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
		UYogUIManagerSubsystem* UIManager = LocalPlayer ? LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>() : nullptr;
		if (!UIManager)
		{
			return nullptr;
		}

		FYogUIScreenInputPolicy Policy;
		Policy.bShowMouseCursor = true;
		Policy.bPauseGame = bPauseGame;
		Policy.bAffectsMajorUI = false;
		UIManager->SetInputPolicyOverride(EYogUIScreenId::TutorialPopup, Policy);

		if (FallbackWidget)
		{
			UIManager->SetWidgetClassOverride(EYogUIScreenId::TutorialPopup, FallbackWidget->GetClass());
		}

		return UIManager;
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
	// If the widget instance changed (level transition → new HUD), reset popup showing state
	// so a stale bPopupShowing=true from the previous level doesn't block all new tutorials.
	if (PopupWidget.Get() != InWidget)
	{
		if (bPopupShowing)
		{
			UE_LOG(LogTemp, Log, TEXT("[Tutorial] Init: widget changed during popup (level transition?) — resetting bPopupShowing."));
		}
		bPopupShowing = false;

		if (DilationTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(DilationTickerHandle);
			DilationTickerHandle.Reset();
		}
		EndDilationVisualIfActive();
	}

	PopupWidget = InWidget;
	Registry = ResolveTutorialRegistry(InRegistry);

	UE_LOG(LogTemp, Log, TEXT("[Tutorial] Init: PopupWidget=%s, Registry=%s"),
		InWidget ? *InWidget->GetClass()->GetName() : TEXT("NULL - TutorialPopupClass is not set on BP_HUD"),
		Registry ? *Registry->GetName() : TEXT("NULL - TutorialRegistry is not set on BP_HUD"));
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

	if (!AreTutorialPopupsEnabled())
	{
		UE_LOG(LogTemp, Log, TEXT("[Tutorial] Weapon tutorial suppressed because tutorial popups are disabled."));
		return;
	}

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
	if (!AreTutorialPopupsEnabled())
	{
		return;
	}
	if (!WeakPC.IsValid() || !PopupWidget.IsValid())
	{
		return;
	}

	TArray<FTutorialPage> PagesToShow;
	if (Registry)
	{
		if (const TArray<FTutorialPage>* RegisteredPages = Registry->FindPages(WeaponPickupTutorialEventID))
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
			TEXT("靠近发光武器，按 <input action=\"Interact\"/> 拾取。武器会装备到角色身上，并把它自带的初始卡牌装入下方 1D 卡组。")));
		PagesToShow.Add(MakeTutorialPage(
			TEXT("武器自带初始卡牌"),
			TEXT("每把武器都有一组起始卡。拾取后，卡组会按武器配置顺序显示；轻/重攻击不会被卡组阻止，但命中会按顺序消耗卡牌并触发效果。")));
		PagesToShow.Add(MakeTutorialPage(
			TEXT("试试攻击木头人"),
			TEXT("拾取武器后，走到木头人身边，尝试用 <input action=\"LightAttack\"/> 或 <input action=\"HeavyAttack\"/> 攻击它。击败后会掉落一张新卡牌，靠近掉落物再按 <input action=\"Interact\"/> 拾取。")));
	}

	bPopupShowing = true;
	UYogUIManagerSubsystem* UIManager = PrepareTutorialPopupManager(PopupWidget.Get(), WeakPC.Get(), true);
	UTutorialPopupWidget* Widget = UIManager
		? Cast<UTutorialPopupWidget>(UIManager->EnsureWidget(EYogUIScreenId::TutorialPopup))
		: PopupWidget.Get();
	PopupWidget = Widget;
	if (Widget)
	{
		Widget->ShowPopup(PagesToShow);
		if (UIManager)
		{
			UIManager->PushScreen(EYogUIScreenId::TutorialPopup);
		}
	}

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

	if (ShowByEventID(ResolveLinkCardTutorialEventId(), PC, /*bPauseGame=*/true))
	{
		State = ETutorialState::Completed;
		SaveState();
	}
}

void UTutorialManager::NotifyLinkCardEnteredDeck(APlayerController* PC)
{
	if (!AreTutorialPopupsEnabled())
	{
		return;
	}

	const FGameplayTag HintTag = GetLinkCardHintTag();
	if (!HintTag.IsValid() || HasShownHint(HintTag))
	{
		bLinkCardBackpackTutorialPending = false;
		return;
	}
	if (bLinkCardBackpackTutorialPending)
	{
		return;
	}

	bLinkCardBackpackTutorialPending = true;
	ShowLinkCardBackpackPrompt(PC);
}

bool UTutorialManager::TryShowPendingLinkCardTutorial(APlayerController* PC)
{
	if (!bLinkCardBackpackTutorialPending)
	{
		return false;
	}

	const FGameplayTag HintTag = GetLinkCardHintTag();
	if (!HintTag.IsValid() || HasShownHint(HintTag))
	{
		bLinkCardBackpackTutorialPending = false;
		return true;
	}

	if (TryShowHintOnce(HintTag, ResolveLinkCardTutorialEventId(), PC, /*bPauseGame=*/true))
	{
		bLinkCardBackpackTutorialPending = false;
		return true;
	}

	return false;
}

FName UTutorialManager::ResolveLinkCardTutorialEventIdForTest(const UTutorialRegistryDA* InRegistry)
{
	return ResolveLinkCardTutorialEventIdFromRegistry(InRegistry);
}

UTutorialRegistryDA* UTutorialManager::ResolveTutorialRegistry(UTutorialRegistryDA* InRegistry)
{
	if (InRegistry)
	{
		return InRegistry;
	}

	return LoadObject<UTutorialRegistryDA>(nullptr, DefaultTutorialRegistryObjectPath);
}

UTutorialRegistryDA* UTutorialManager::ResolveTutorialRegistryForTest(UTutorialRegistryDA* InRegistry)
{
	return ResolveTutorialRegistry(InRegistry);
}

bool UTutorialManager::IsDirectEventTutorialAllowed(FName EventID)
{
	return EventID != WeaponPickupTutorialEventID;
}

bool UTutorialManager::IsDirectEventTutorialAllowedForTest(FName EventID)
{
	return IsDirectEventTutorialAllowed(EventID);
}

FName UTutorialManager::ResolveLinkCardTutorialEventId() const
{
	return ResolveLinkCardTutorialEventIdFromRegistry(Registry);
}

void UTutorialManager::ShowLinkCardBackpackPrompt(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD());
	if (!HUD)
	{
		return;
	}

	ULevelInfoPopupDA* Popup = NewObject<ULevelInfoPopupDA>(this);
	Popup->Title = FText::FromString(TEXT("获得连携卡"));
	Popup->Body = FText::FromString(TEXT("月光已进入卡组。打开背包查看连携卡的正向/反向规则。"));
	Popup->HUDSummaryText = FText::FromString(TEXT("打开背包查看月光连携卡的正向/反向规则。"));
	Popup->DisplayDuration = 4.0f;

	TransientInfoPopups.Add(Popup);
	if (TransientInfoPopups.Num() > 8)
	{
		TransientInfoPopups.RemoveAt(0);
	}

	HUD->ShowInfoPopup(Popup);
}

bool UTutorialManager::ShowInlinePages(const TArray<FTutorialPage>& Pages, APlayerController* PC, bool bPauseGame)
{
	if (!AreTutorialPopupsEnabled())
	{
		UE_LOG(LogTemp, Log, TEXT("[Tutorial] Inline tutorial suppressed because tutorial popups are disabled."));
		OnPopupClosed.Broadcast();
		return false;
	}

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
	UYogUIManagerSubsystem* UIManager = PrepareTutorialPopupManager(PopupWidget.Get(), PC, bPauseGame);
	UTutorialPopupWidget* Widget = UIManager
		? Cast<UTutorialPopupWidget>(UIManager->EnsureWidget(EYogUIScreenId::TutorialPopup))
		: PopupWidget.Get();
	PopupWidget = Widget;
	if (Widget)
	{
		Widget->ShowPopup(Pages, bPauseGame);
		if (UIManager)
		{
			UIManager->PushScreen(EYogUIScreenId::TutorialPopup);
		}
	}
	return true;
}

bool UTutorialManager::ShowByEventID(FName EventID, APlayerController* PC, bool bPauseGame)
{
	if (!AreTutorialPopupsEnabled())
	{
		UE_LOG(LogTemp, Log, TEXT("[Tutorial] EventID='%s' suppressed because tutorial popups are disabled."), *EventID.ToString());
		OnPopupClosed.Broadcast();
		return false;
	}

	if (!IsDirectEventTutorialAllowed(EventID))
	{
		UE_LOG(LogTemp, Log, TEXT("[Tutorial] EventID='%s' suppressed because it must be triggered by the weapon pickup flow."),
			*EventID.ToString());
		OnPopupClosed.Broadcast();
		return false;
	}

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
		BuildFallbackPagesForEventID(EventID, PagesToShow);
	}

	if (PagesToShow.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Tutorial] EventID='%s' has no tutorial pages; popup skipped."), *EventID.ToString());
		OnPopupClosed.Broadcast();
		return false;
	}

	bPopupShowing = true;
	UYogUIManagerSubsystem* UIManager = PrepareTutorialPopupManager(PopupWidget.Get(), PC, bPauseGame);
	UTutorialPopupWidget* Widget = UIManager
		? Cast<UTutorialPopupWidget>(UIManager->EnsureWidget(EYogUIScreenId::TutorialPopup))
		: PopupWidget.Get();
	PopupWidget = Widget;
	if (Widget)
	{
		Widget->ShowPopup(PagesToShow, bPauseGame);
		if (UIManager)
		{
			UIManager->PushScreen(EYogUIScreenId::TutorialPopup);
		}
	}
	return true;
}

void UTutorialManager::NotifyPopupClosed()
{
	if (!bPopupShowing)
	{
		return;
	}

	bPopupShowing = false;
	OnPopupClosed.Broadcast();
	FlushPendingHints();
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

// ─────────────────────────────────────────────────────────────────────────────
//  事件驱动一次性提示（HintOnce）
//  不依赖 ETutorialState 顺序；每个 HintTag 只展示一次，结果写入 ShownPopupKeys。
//  当前有弹窗时进入 PendingHints 队列，NotifyPopupClosed 后自动补发。
// ─────────────────────────────────────────────────────────────────────────────

bool UTutorialManager::HasShownHint(FGameplayTag HintTag) const
{
	if (!HintTag.IsValid()) return false;
	UGameInstance* GI = GetGameInstance();
	UYogSaveSubsystem* SaveSys = GI ? GI->GetSubsystem<UYogSaveSubsystem>() : nullptr;
	const UYogSaveGame* Save = SaveSys ? SaveSys->GetCurrentSave() : nullptr;
	return Save && Save->ShownPopupKeys.Contains(HintTag);
}

void UTutorialManager::MarkHintShown(FGameplayTag HintTag)
{
	if (!HintTag.IsValid()) return;
	UGameInstance* GI = GetGameInstance();
	UYogSaveSubsystem* SaveSys = GI ? GI->GetSubsystem<UYogSaveSubsystem>() : nullptr;
	UYogSaveGame* Save = SaveSys ? SaveSys->GetCurrentSave() : nullptr;
	if (Save)
	{
		Save->ShownPopupKeys.Add(HintTag);
		SaveSys->WriteSaveGame();
	}
}

bool UTutorialManager::TryShowHintOnce(FGameplayTag HintTag, FName EventID, APlayerController* PC, bool bPauseGame)
{
	if (!AreTutorialPopupsEnabled()) return false;
	if (HasShownHint(HintTag))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Tutorial] HintOnce '%s' already shown, skipping."), *HintTag.ToString());
		return true;
	}

	// 同一 HintTag 不重复入队
	for (const FPendingHint& Pending : PendingHints)
	{
		if (Pending.HintTag == HintTag) return true;
	}

	if (!bPopupShowing)
	{
		if (ShowByEventID(EventID, PC, bPauseGame))
		{
			MarkHintShown(HintTag);
			return true;
		}
		// ShowByEventID 失败（EventID 未配置等），不入队
		UE_LOG(LogTemp, Warning, TEXT("[Tutorial] HintOnce '%s' ShowByEventID('%s') failed immediately, not queued."),
			*HintTag.ToString(), *EventID.ToString());
		return false;
	}

	// 当前有弹窗，进入队列等待补发
	UE_LOG(LogTemp, Log, TEXT("[Tutorial] HintOnce '%s' queued (popup busy)."), *HintTag.ToString());
	FPendingHint& Queued = PendingHints.AddDefaulted_GetRef();
	Queued.HintTag   = HintTag;
	Queued.EventID   = EventID;
	Queued.PC        = PC;
	Queued.bPauseGame = bPauseGame;
	return true;
}

void UTutorialManager::FlushPendingHints()
{
	while (!PendingHints.IsEmpty() && !bPopupShowing)
	{
		FPendingHint Next = PendingHints[0];
		PendingHints.RemoveAt(0);

		if (HasShownHint(Next.HintTag))
		{
			// 队列期间已被其他路径标记完成，跳过
			continue;
		}
		if (!Next.PC.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Tutorial] FlushPendingHints: PC invalid for hint '%s', dropping."),
				*Next.HintTag.ToString());
			continue;
		}

		if (ShowByEventID(Next.EventID, Next.PC.Get(), Next.bPauseGame))
		{
			MarkHintShown(Next.HintTag);
			// ShowByEventID 会把 bPopupShowing 设为 true，循环下次判断时退出
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────

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
