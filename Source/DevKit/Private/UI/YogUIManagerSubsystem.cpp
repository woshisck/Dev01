#include "UI/YogUIManagerSubsystem.h"

#include "DevAssetManager.h"
#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"

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
	BoundActivationScreens.Reset();
	ActivatedScreens.Reset();
	PendingAsyncLoads.Reset();
	TopActivatedLayer = EYogUILayer::Game;

	Super::Deinitialize();
}

UYogUIRegistry* UYogUIManagerSubsystem::GetRegistry() const
{
	return CachedRegistry;
}

TSubclassOf<UUserWidget> UYogUIManagerSubsystem::GetWidgetClass(EYogUIScreenId ScreenId) const
{
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

UUserWidget* UYogUIManagerSubsystem::EnsureWidget(EYogUIScreenId ScreenId)
{
	if (TObjectPtr<UUserWidget>* Existing = Instances.Find(ScreenId))
	{
		if (*Existing && (*Existing)->IsInViewport())
		{
			return *Existing;
		}
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
	RecomputeTopLayer();
}

void UYogUIManagerSubsystem::HandleScreenDeactivated(EYogUIScreenId ScreenId)
{
	ActivatedScreens.Remove(ScreenId);
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

void UYogUIManagerSubsystem::ApplyInputModeForLayer(EYogUILayer NewLayer)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	if (NewLayer == EYogUILayer::Game)
	{
		FInputModeGameOnly Mode;
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = false;
		return;
	}

	// Menu / Modal：找当前最高 Layer 中的一个 widget 作为焦点目标。
	UCommonActivatableWidget* FocusTarget = nullptr;
	for (EYogUIScreenId ScreenId : ActivatedScreens)
	{
		if (GetLayerForScreen(ScreenId) != NewLayer)
		{
			continue;
		}
		if (UCommonActivatableWidget* W = Cast<UCommonActivatableWidget>(GetWidget(ScreenId)))
		{
			FocusTarget = W;
			break;
		}
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
	PC->bShowMouseCursor = (NewLayer == EYogUILayer::Modal);
}
