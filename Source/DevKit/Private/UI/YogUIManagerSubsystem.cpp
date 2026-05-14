#include "UI/YogUIManagerSubsystem.h"

#include "DevAssetManager.h"
#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "UI/YogHUD.h"

namespace
{
bool IsInteractiveManagedScreen(EYogUIScreenId ScreenId)
{
	switch (ScreenId)
	{
	case EYogUIScreenId::Backpack:
	case EYogUIScreenId::LootSelection:
	case EYogUIScreenId::PauseMenu:
	case EYogUIScreenId::TutorialPopup:
	case EYogUIScreenId::SacrificeGraceOption:
	case EYogUIScreenId::ShopSelection:
	case EYogUIScreenId::AltarMenu:
	case EYogUIScreenId::SacrificeSelection:
	case EYogUIScreenId::RunePurification:
	case EYogUIScreenId::EntryMenu:
		return true;
	default:
		return false;
	}
}
}

void UYogUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CachedRegistry = UDevAssetManager::Get().GetUIRegistry();
}

void UYogUIManagerSubsystem::Deinitialize()
{
	for (auto& Pair : Instances)
	{
		if (UUserWidget* W = Pair.Value)
		{
			W->RemoveFromParent();
		}
	}
	Instances.Reset();
	LoadedWidgetClasses.Reset();
	WidgetClassOverrides.Reset();
	InputPolicyOverrides.Reset();
	BoundActivationScreens.Reset();
	ActivatedScreens.Reset();
	ActivationStack.Reset();
	PendingAsyncLoads.Reset();
	ShownPopups_Session.Reset();
	ShownPopups_Run.Reset();
	PendingPopupOnce.Reset();
	TopActivatedLayer = EYogUILayer::Game;
	bManagedPauseActive = false;
	bManagedMajorUIActive = false;
	bManagedPawnInputDisabled = false;

	Super::Deinitialize();
}

UYogUIRegistry* UYogUIManagerSubsystem::GetRegistry() const
{
	return CachedRegistry;
}

TSubclassOf<UUserWidget> UYogUIManagerSubsystem::GetWidgetClass(EYogUIScreenId ScreenId) const
{
	if (const TSubclassOf<UUserWidget>* OverrideClass = WidgetClassOverrides.Find(ScreenId))
	{
		return *OverrideClass;
	}

	if (const TSubclassOf<UUserWidget>* CachedClass = LoadedWidgetClasses.Find(ScreenId))
	{
		return *CachedClass;
	}

	if (!CachedRegistry)
	{
		return nullptr;
	}

	FYogUIRegistryEntry Entry;
	if (!CachedRegistry->FindEntry(ScreenId, Entry) || Entry.WidgetClass.IsNull())
	{
		return nullptr;
	}

	TSubclassOf<UUserWidget> LoadedClass = UDevAssetManager::GetSubclass<UUserWidget>(Entry.WidgetClass);
	if (LoadedClass)
	{
		LoadedWidgetClasses.Add(ScreenId, LoadedClass);
	}

	return LoadedClass;
}

int32 UYogUIManagerSubsystem::GetZOrder(EYogUIScreenId ScreenId, int32 FallbackZOrder) const
{
	return CachedRegistry ? CachedRegistry->GetZOrder(ScreenId, FallbackZOrder) : FallbackZOrder;
}

void UYogUIManagerSubsystem::SetWidgetClassOverride(EYogUIScreenId ScreenId, TSubclassOf<UUserWidget> WidgetClass)
{
	if (WidgetClass)
	{
		WidgetClassOverrides.Add(ScreenId, WidgetClass);
		LoadedWidgetClasses.Remove(ScreenId);
		if (TObjectPtr<UUserWidget>* Existing = Instances.Find(ScreenId))
		{
			if (*Existing && !(*Existing)->IsA(WidgetClass))
			{
				(*Existing)->RemoveFromParent();
				Instances.Remove(ScreenId);
				BoundActivationScreens.Remove(ScreenId);
				ActivatedScreens.Remove(ScreenId);
				ActivationStack.Remove(ScreenId);
				if (bAutoManageInputMode)
				{
					RecomputeTopLayer();
				}
			}
		}
	}
	else
	{
		WidgetClassOverrides.Remove(ScreenId);
	}
}

void UYogUIManagerSubsystem::SetInputPolicyOverride(EYogUIScreenId ScreenId, const FYogUIScreenInputPolicy& Policy)
{
	InputPolicyOverrides.Add(ScreenId, Policy);
	if (ActivatedScreens.Contains(ScreenId) && bAutoManageInputMode)
	{
		RecomputeTopLayer();
	}
}

void UYogUIManagerSubsystem::ClearInputPolicyOverride(EYogUIScreenId ScreenId)
{
	InputPolicyOverrides.Remove(ScreenId);
	if (ActivatedScreens.Contains(ScreenId) && bAutoManageInputMode)
	{
		RecomputeTopLayer();
	}
}

APlayerController* UYogUIManagerSubsystem::GetOwningPlayerController() const
{
	if (const ULocalPlayer* LP = GetLocalPlayer())
	{
		return LP->GetPlayerController(LP->GetWorld());
	}
	return nullptr;
}

EYogUILayer UYogUIManagerSubsystem::GetLayerForScreen(EYogUIScreenId ScreenId) const
{
	if (IsInteractiveManagedScreen(ScreenId))
	{
		return ScreenId == EYogUIScreenId::PauseMenu ? EYogUILayer::Modal : EYogUILayer::Menu;
	}

	if (!CachedRegistry)
	{
		return EYogUILayer::Game;
	}

	FYogUIRegistryEntry Entry;
	if (CachedRegistry->FindEntry(ScreenId, Entry))
	{
		return Entry.Layer;
	}
	return EYogUILayer::Game;
}

FYogUIScreenInputPolicy UYogUIManagerSubsystem::GetInputPolicyForScreen(EYogUIScreenId ScreenId) const
{
	if (const FYogUIScreenInputPolicy* OverridePolicy = InputPolicyOverrides.Find(ScreenId))
	{
		return *OverridePolicy;
	}

	FYogUIScreenInputPolicy Policy;
	const EYogUILayer Layer = GetLayerForScreen(ScreenId);
	if (Layer == EYogUILayer::Menu || Layer == EYogUILayer::Modal)
	{
		Policy.bShowMouseCursor = true;
		Policy.bPauseGame = true;
		Policy.bAffectsMajorUI = ScreenId != EYogUIScreenId::TutorialPopup && ScreenId != EYogUIScreenId::PauseMenu;
	}
	if (ScreenId == EYogUIScreenId::LootSelection)
	{
		Policy.bDisablePawnInput = true;
	}

	if (CachedRegistry)
	{
		FYogUIRegistryEntry Entry;
		if (CachedRegistry->FindEntry(ScreenId, Entry) && Entry.bOverrideInputPolicy)
		{
			Policy.bShowMouseCursor = Entry.bShowMouseCursor;
			Policy.bPauseGame = Entry.bPauseGame;
			Policy.bDisablePawnInput = Entry.bDisablePawnInput;
			Policy.bAffectsMajorUI = Entry.bAffectsMajorUI;
		}
	}

	return Policy;
}

UUserWidget* UYogUIManagerSubsystem::EnsureWidget(EYogUIScreenId ScreenId)
{
	if (TObjectPtr<UUserWidget>* Existing = Instances.Find(ScreenId))
	{
		if (*Existing && (*Existing)->IsInViewport())
		{
			return *Existing;
		}
		Instances.Remove(ScreenId);
		BoundActivationScreens.Remove(ScreenId);
		ActivatedScreens.Remove(ScreenId);
		ActivationStack.Remove(ScreenId);
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	TSubclassOf<UUserWidget> WidgetClass = GetWidgetClass(ScreenId);
	if (!WidgetClass)
	{
		return nullptr;
	}

	UUserWidget* W = CreateWidget<UUserWidget>(PC, WidgetClass);
	if (!W)
	{
		return nullptr;
	}

	W->AddToViewport(GetZOrder(ScreenId, 0));
	Instances.Add(ScreenId, W);

	if (UCommonActivatableWidget* Activatable = Cast<UCommonActivatableWidget>(W))
	{
		BindActivationDelegates(ScreenId, Activatable);
	}
	return W;
}

void UYogUIManagerSubsystem::BindActivationDelegates(EYogUIScreenId ScreenId, UCommonActivatableWidget* Activatable)
{
	if (!Activatable || BoundActivationScreens.Contains(ScreenId))
	{
		return;
	}

	TWeakObjectPtr<UYogUIManagerSubsystem> WeakThis(this);
	Activatable->OnActivated().AddWeakLambda(this, [WeakThis, ScreenId]()
	{
		if (UYogUIManagerSubsystem* S = WeakThis.Get())
		{
			S->HandleScreenActivated(ScreenId);
		}
	});
	Activatable->OnDeactivated().AddWeakLambda(this, [WeakThis, ScreenId]()
	{
		if (UYogUIManagerSubsystem* S = WeakThis.Get())
		{
			S->HandleScreenDeactivated(ScreenId);
		}
	});
	BoundActivationScreens.Add(ScreenId);
}

UCommonActivatableWidget* UYogUIManagerSubsystem::PushScreen(EYogUIScreenId ScreenId)
{
	UUserWidget* W = EnsureWidget(ScreenId);
	UCommonActivatableWidget* Activatable = Cast<UCommonActivatableWidget>(W);
	if (!Activatable)
	{
		return nullptr;
	}

	BindActivationDelegates(ScreenId, Activatable);

	if (!Activatable->IsActivated())
	{
		Activatable->ActivateWidget();
	}
	else
	{
		HandleScreenActivated(ScreenId);
	}
	return Activatable;
}

void UYogUIManagerSubsystem::PushScreenAsync(EYogUIScreenId ScreenId, const FOnAsyncScreenReady& OnReady)
{
	// 已经加载好：走同步路径
	if (LoadedWidgetClasses.Contains(ScreenId))
	{
		UCommonActivatableWidget* W = PushScreen(ScreenId);
		OnReady.ExecuteIfBound(ScreenId, W);
		return;
	}

	if (!CachedRegistry)
	{
		OnReady.ExecuteIfBound(ScreenId, nullptr);
		return;
	}

	FYogUIRegistryEntry Entry;
	if (!CachedRegistry->FindEntry(ScreenId, Entry) || Entry.WidgetClass.IsNull())
	{
		OnReady.ExecuteIfBound(ScreenId, nullptr);
		return;
	}

	// 已经在加载中：覆盖 callback（最近一个 caller 拿到结果）
	if (PendingAsyncLoads.Contains(ScreenId))
	{
		PendingAsyncLoads.Add(ScreenId, OnReady);
		return;
	}
	PendingAsyncLoads.Add(ScreenId, OnReady);

	const FSoftObjectPath Path = Entry.WidgetClass.ToSoftObjectPath();
	TWeakObjectPtr<UYogUIManagerSubsystem> WeakThis(this);
	UAssetManager::GetStreamableManager().RequestAsyncLoad(Path,
		FStreamableDelegate::CreateLambda([WeakThis, ScreenId]()
		{
			UYogUIManagerSubsystem* S = WeakThis.Get();
			if (!S)
			{
				return;
			}

			FOnAsyncScreenReady Cb;
			S->PendingAsyncLoads.RemoveAndCopyValue(ScreenId, Cb);

			UCommonActivatableWidget* W = S->PushScreen(ScreenId);
			Cb.ExecuteIfBound(ScreenId, W);
		}));
}

UYogSaveGame* UYogUIManagerSubsystem::GetCurrentSaveGame() const
{
	const ULocalPlayer* LP = GetLocalPlayer();
	if (!LP)
	{
		return nullptr;
	}
	const UGameInstance* GI = LP->GetGameInstance();
	if (!GI)
	{
		return nullptr;
	}
	UYogSaveSubsystem* SaveSys = GI->GetSubsystem<UYogSaveSubsystem>();
	return SaveSys ? SaveSys->GetCurrentSave() : nullptr;
}

bool UYogUIManagerSubsystem::IsPopupShown(FGameplayTag PopupKey, EPopupScope Scope) const
{
	if (!PopupKey.IsValid())
	{
		return false;
	}

	switch (Scope)
	{
	case EPopupScope::Session:
		return ShownPopups_Session.Contains(PopupKey);
	case EPopupScope::Run:
		return ShownPopups_Run.Contains(PopupKey);
	case EPopupScope::Save:
		if (const UYogSaveGame* Save = GetCurrentSaveGame())
		{
			return Save->ShownPopupKeys.Contains(PopupKey);
		}
		return false;
	}
	return false;
}

void UYogUIManagerSubsystem::MarkPopupShown(FGameplayTag PopupKey, EPopupScope Scope)
{
	if (!PopupKey.IsValid())
	{
		return;
	}

	switch (Scope)
	{
	case EPopupScope::Session:
		ShownPopups_Session.Add(PopupKey);
		break;
	case EPopupScope::Run:
		ShownPopups_Run.Add(PopupKey);
		break;
	case EPopupScope::Save:
		if (UYogSaveGame* Save = GetCurrentSaveGame())
		{
			bool bAlreadyShown = false;
			Save->ShownPopupKeys.Add(PopupKey, &bAlreadyShown);
			if (!bAlreadyShown)
			{
				if (const ULocalPlayer* LP = GetLocalPlayer())
				{
					if (const UGameInstance* GI = LP->GetGameInstance())
					{
						if (UYogSaveSubsystem* SaveSys = GI->GetSubsystem<UYogSaveSubsystem>())
						{
							SaveSys->WriteSaveGame();
						}
					}
				}
			}
		}
		break;
	}
}

void UYogUIManagerSubsystem::ResetPopupsForScope(EPopupScope Scope)
{
	switch (Scope)
	{
	case EPopupScope::Session:
		ShownPopups_Session.Reset();
		break;
	case EPopupScope::Run:
		ShownPopups_Run.Reset();
		break;
	case EPopupScope::Save:
		if (UYogSaveGame* Save = GetCurrentSaveGame())
		{
			Save->ShownPopupKeys.Reset();
			if (const ULocalPlayer* LP = GetLocalPlayer())
			{
				if (const UGameInstance* GI = LP->GetGameInstance())
				{
					if (UYogSaveSubsystem* SaveSys = GI->GetSubsystem<UYogSaveSubsystem>())
					{
						SaveSys->WriteSaveGame();
					}
				}
			}
		}
		break;
	}
}

UCommonActivatableWidget* UYogUIManagerSubsystem::PushScreenOnce(EYogUIScreenId ScreenId, FGameplayTag PopupKey, EPopupScope Scope)
{
	if (!PopupKey.IsValid())
	{
		// 没 key 就退回到普通 PushScreen，避免悄悄把所有"忘了配 Tag"的弹窗都吞掉。
		return PushScreen(ScreenId);
	}

	if (IsPopupShown(PopupKey, Scope))
	{
		return nullptr;
	}

	UCommonActivatableWidget* W = PushScreen(ScreenId);
	if (W)
	{
		MarkPopupShown(PopupKey, Scope);
	}
	return W;
}

void UYogUIManagerSubsystem::PushScreenOnceAsync(EYogUIScreenId ScreenId, FGameplayTag PopupKey, EPopupScope Scope, const FOnAsyncScreenReady& OnReady)
{
	if (!PopupKey.IsValid())
	{
		PushScreenAsync(ScreenId, OnReady);
		return;
	}

	if (IsPopupShown(PopupKey, Scope))
	{
		OnReady.ExecuteIfBound(ScreenId, nullptr);
		return;
	}

	FPendingPopupOnce Pending;
	Pending.Key = PopupKey;
	Pending.Scope = Scope;
	Pending.UserCallback = OnReady;
	PendingPopupOnce.Add(ScreenId, MoveTemp(Pending));

	FOnAsyncScreenReady Wrapped;
	Wrapped.BindDynamic(this, &UYogUIManagerSubsystem::HandlePushScreenOnceAsyncReady);
	PushScreenAsync(ScreenId, Wrapped);
}

void UYogUIManagerSubsystem::HandlePushScreenOnceAsyncReady(EYogUIScreenId ScreenId, UCommonActivatableWidget* Widget)
{
	FPendingPopupOnce Pending;
	if (!PendingPopupOnce.RemoveAndCopyValue(ScreenId, Pending))
	{
		return;
	}

	if (Widget)
	{
		MarkPopupShown(Pending.Key, Pending.Scope);
	}
	Pending.UserCallback.ExecuteIfBound(ScreenId, Widget);
}

void UYogUIManagerSubsystem::PopScreen(EYogUIScreenId ScreenId)
{
	if (TObjectPtr<UUserWidget>* Existing = Instances.Find(ScreenId))
	{
		if (UCommonActivatableWidget* Activatable = Cast<UCommonActivatableWidget>(*Existing))
		{
			if (Activatable->IsActivated())
			{
				Activatable->DeactivateWidget();
			}
		}
	}
}

bool UYogUIManagerSubsystem::PopManagedScreen(UUserWidget* Widget, EYogUIScreenId ScreenId)
{
	if (!Widget)
	{
		return false;
	}

	if (ULocalPlayer* LocalPlayer = Widget->GetOwningLocalPlayer())
	{
		if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
		{
			UIManager->PopScreen(ScreenId);
			return true;
		}
	}
	return false;
}

UUserWidget* UYogUIManagerSubsystem::GetWidget(EYogUIScreenId ScreenId) const
{
	if (const TObjectPtr<UUserWidget>* Existing = Instances.Find(ScreenId))
	{
		return *Existing;
	}
	return nullptr;
}

bool UYogUIManagerSubsystem::IsScreenActive(EYogUIScreenId ScreenId) const
{
	if (const TObjectPtr<UUserWidget>* Existing = Instances.Find(ScreenId))
	{
		if (const UCommonActivatableWidget* Activatable = Cast<UCommonActivatableWidget>(*Existing))
		{
			return Activatable->IsActivated();
		}
	}
	return false;
}

void UYogUIManagerSubsystem::CreateAutoStartWidgets()
{
	if (!CachedRegistry)
	{
		return;
	}

	for (const FYogUIRegistryEntry& Entry : CachedRegistry->Entries)
	{
		if (!Entry.bCreateOnHUDStart)
		{
			continue;
		}
		EnsureWidget(Entry.ScreenId);
	}
}

void UYogUIManagerSubsystem::HandleScreenActivated(EYogUIScreenId ScreenId)
{
	ActivatedScreens.Add(ScreenId);
	ActivationStack.Remove(ScreenId);
	ActivationStack.Add(ScreenId);
	RecomputeTopLayer();
}

void UYogUIManagerSubsystem::HandleScreenDeactivated(EYogUIScreenId ScreenId)
{
	ActivatedScreens.Remove(ScreenId);
	ActivationStack.Remove(ScreenId);
	RecomputeTopLayer();
}

void UYogUIManagerSubsystem::RecomputeTopLayer()
{
	EYogUILayer NewTop = EYogUILayer::Game;
	for (EYogUIScreenId ScreenId : ActivatedScreens)
	{
		const EYogUILayer L = GetLayerForScreen(ScreenId);
		if ((uint8)L > (uint8)NewTop)
		{
			NewTop = L;
		}
	}

	if (NewTop != TopActivatedLayer)
	{
		TopActivatedLayer = NewTop;
		OnTopLayerChanged.Broadcast(NewTop);
	}

	// InputMode 每次都重算一次：即便 Layer 没变，顶层 widget 可能切换了，焦点也要跟着走。
	if (bAutoManageInputMode)
	{
		ApplyInputModeForLayer(NewTop);
	}
}

EYogUIScreenId UYogUIManagerSubsystem::FindTopActiveScreen(EYogUILayer NewLayer) const
{
	for (int32 Index = ActivationStack.Num() - 1; Index >= 0; --Index)
	{
		const EYogUIScreenId ScreenId = ActivationStack[Index];
		if (ActivatedScreens.Contains(ScreenId) && GetLayerForScreen(ScreenId) == NewLayer)
		{
			return ScreenId;
		}
	}
	return EYogUIScreenId::MainHUD;
}

void UYogUIManagerSubsystem::ApplyPauseAndMajorUIState(const FYogUIScreenInputPolicy& Policy)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD());
	if (Policy.bPauseGame != bManagedPauseActive)
	{
		if (Policy.bPauseGame)
		{
			if (HUD)
			{
				HUD->BeginPauseEffect();
			}
			else
			{
				PC->SetPause(true);
			}
		}
		else
		{
			if (HUD)
			{
				HUD->EndPauseEffect();
			}
			else
			{
				PC->SetPause(false);
			}
		}
		bManagedPauseActive = Policy.bPauseGame;
	}

	if (Policy.bAffectsMajorUI != bManagedMajorUIActive)
	{
		if (HUD)
		{
			if (Policy.bAffectsMajorUI)
			{
				HUD->PushMajorUI();
			}
			else
			{
				HUD->PopMajorUI();
			}
		}
		bManagedMajorUIActive = Policy.bAffectsMajorUI;
	}

	if (Policy.bDisablePawnInput != bManagedPawnInputDisabled)
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (Policy.bDisablePawnInput)
			{
				Pawn->DisableInput(PC);
			}
			else
			{
				Pawn->EnableInput(PC);
			}
		}
		if (Policy.bDisablePawnInput)
		{
			PC->DisableInput(PC);
		}
		else
		{
			PC->EnableInput(PC);
		}
		bManagedPawnInputDisabled = Policy.bDisablePawnInput;
	}
}

void UYogUIManagerSubsystem::ApplyInputModeForLayer(EYogUILayer NewLayer)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	if (NewLayer == EYogUILayer::Game)
	{
		ApplyPauseAndMajorUIState(FYogUIScreenInputPolicy());
		FInputModeGameOnly Mode;
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = false;
		return;
	}

	// Menu / Modal：找当前最高 Layer 中的一个 widget 作为焦点目标。
	const EYogUIScreenId TopScreenId = FindTopActiveScreen(NewLayer);
	const FYogUIScreenInputPolicy Policy = GetInputPolicyForScreen(TopScreenId);
	ApplyPauseAndMajorUIState(Policy);

	UCommonActivatableWidget* FocusTarget = nullptr;
	if (TopScreenId != EYogUIScreenId::MainHUD)
	{
		FocusTarget = Cast<UCommonActivatableWidget>(GetWidget(TopScreenId));
	}

	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);

	if (FocusTarget)
	{
		if (UWidget* Desired = FocusTarget->GetDesiredFocusTarget())
		{
			Mode.SetWidgetToFocus(Desired->TakeWidget());
		}
		else
		{
			Mode.SetWidgetToFocus(FocusTarget->TakeWidget());
		}
	}

	PC->SetInputMode(Mode);
	PC->bShowMouseCursor = Policy.bShowMouseCursor;
}
