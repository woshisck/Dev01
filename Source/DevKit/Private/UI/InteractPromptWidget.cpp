#include "UI/InteractPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const TCHAR* InteractPrompt_HoldRingMaterialPath = TEXT("/Game/UI/Playtest_UI/Interact/M_InteractHoldRing.M_InteractHoldRing");
}

void UInteractPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Build here rather than in NativeConstruct: RebuildWidget runs from TakeWidget, and by
	// NativeConstruct the slate widget has already been built from an empty WidgetTree.
	BuildFallbackLayout();
}

void UInteractPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildFallbackLayout();
	SetHoldProgress(0.f);

	if (UCommonInputSubsystem* InputSub =
		ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(GetOwningLocalPlayer()))
	{
		InputSub->OnInputMethodChangedNative.AddUObject(this, &UInteractPromptWidget::RefreshIcon);
		RefreshIcon(InputSub->GetCurrentInputType());
	}
	else
	{
		RefreshIcon();
	}
}

void UInteractPromptWidget::NativeDestruct()
{
	if (UCommonInputSubsystem* InputSub =
		ULocalPlayer::GetSubsystem<UCommonInputSubsystem>(GetOwningLocalPlayer()))
	{
		InputSub->OnInputMethodChangedNative.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UInteractPromptWidget::SetHoldProgress(float Normalized)
{
	if (!HoldProgressRing)
	{
		return;
	}

	static const FName PercentParam(TEXT("Percent"));

	// GetDynamicMaterial swaps the brush resource for a per-instance MID on first call, so each
	// prompt animates its own ring instead of sharing one parameter across every interactable.
	if (UMaterialInstanceDynamic* RingMaterial = HoldProgressRing->GetDynamicMaterial())
	{
		RingMaterial->SetScalarParameterValue(PercentParam, FMath::Clamp(Normalized, 0.f, 1.f));
	}
}

void UInteractPromptWidget::BuildFallbackLayout()
{
	if (ButtonIcon || !WidgetTree)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PromptRoot"));

	// Ring first so it sits behind the glyph.
	if (UMaterialInterface* RingMaterial =
		LoadObject<UMaterialInterface>(nullptr, InteractPrompt_HoldRingMaterialPath))
	{
		HoldProgressRing = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HoldProgressRing"));
		HoldProgressRing->SetBrushFromMaterial(RingMaterial);
		HoldProgressRing->SetDesiredSizeOverride(FVector2D(RingSize, RingSize));

		if (UOverlaySlot* RingSlot = Cast<UOverlaySlot>(Root->AddChildToOverlay(HoldProgressRing)))
		{
			RingSlot->SetHorizontalAlignment(HAlign_Center);
			RingSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	ButtonIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ButtonIcon"));
	ButtonIcon->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
	if (UOverlaySlot* IconSlot = Cast<UOverlaySlot>(Root->AddChildToOverlay(ButtonIcon)))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}

	WidgetTree->RootWidget = Root;
}

void UInteractPromptWidget::RefreshIcon(ECommonInputType NewInputType)
{
	if (!ButtonIcon)
	{
		return;
	}

	const TSoftObjectPtr<UTexture2D>& Wanted =
		NewInputType == ECommonInputType::Gamepad ? GamepadIcon : KeyboardIcon;

	if (UTexture2D* IconTexture = Wanted.LoadSynchronous())
	{
		ButtonIcon->SetBrushFromTexture(IconTexture);
		ButtonIcon->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
	}
}
