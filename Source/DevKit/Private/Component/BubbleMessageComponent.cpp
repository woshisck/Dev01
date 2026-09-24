#include "Component/BubbleMessageComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "UI/BubbleMessageWidget.h"
#include "UI/YogUIManagerSubsystem.h"
#include "UI/YogUIRegistry.h"

namespace
{
	/** Registry-authored WBP if one is configured, else the C++ class and its fallback layout. */
	TSubclassOf<UBubbleMessageWidget> BubbleMessage_ResolveWidgetClass(const UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr)
		{
			if (UYogUIManagerSubsystem* UI = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
			{
				TSubclassOf<UBubbleMessageWidget> Resolved =
					UI->GetTypedWidgetClass<UBubbleMessageWidget>(EYogUIScreenId::BubbleMessage);
				if (Resolved)
				{
					return Resolved;
				}
			}
		}

		return UBubbleMessageWidget::StaticClass();
	}
}

UBubbleMessageComponent::UBubbleMessageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBubbleMessageComponent::OnRegister()
{
	Super::OnRegister();

	EnsureWidgetComponent();
}

void UBubbleMessageComponent::EnsureWidgetComponent()
{
	if (WidgetComponent)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Outered to the actor, not to this component: an actor only registers scene components it owns,
	// so a widget component parented to this component would never render.
	WidgetComponent = NewObject<UWidgetComponent>(Owner, TEXT("BubbleMessageWidgetComp"));
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComponent->SetDrawAtDesiredSize(true);
	WidgetComponent->SetWidgetClass(BubbleMessage_ResolveWidgetClass(GetWorld()));
	WidgetComponent->SetupAttachment(Owner->GetRootComponent());
	WidgetComponent->RegisterComponent();
	WidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, BubbleHeight));

	// Registration makes UWidgetComponent build the widget itself, at the default (visible) Slate
	// visibility. Collapse it here or the bubble is on screen from spawn until the first line.
	EnsureWidget();
	if (UUserWidget* InnerWidget = WidgetComponent->GetWidget())
	{
		InnerWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UBubbleMessageComponent::EnsureWidget()
{
	EnsureWidgetComponent();
	if (!WidgetComponent)
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			WidgetComponent->SetOwnerPlayer(PC->GetLocalPlayer());
		}
	}

	// InitWidget must run exactly once. Calling it again destroys and recreates the widget, leaving
	// the renderer in a freshly-created state that doesn't paint on the first SetVisibility(true).
	if (WidgetComponent->GetWidget())
	{
		return;
	}

	WidgetComponent->SetWidgetClass(BubbleMessage_ResolveWidgetClass(GetWorld()));
	WidgetComponent->InitWidget();
}

void UBubbleMessageComponent::ShowLine(const FText& SpeakerName, const FText& Body)
{
	EnsureWidget();

	UBubbleMessageWidget* BubbleWidget = GetBubbleWidget();
	if (!BubbleWidget)
	{
		return;
	}

	BubbleWidget->SetLine(SpeakerName, Body);
	BubbleWidget->SetFadeAlpha(1.f);

	// The component itself stays visible so it keeps getting added to the screen layer; only the
	// inner widget's Slate visibility is toggled. Toggling component visibility is racy on
	// screen-space widgets because UWidgetComponent::UpdateWidgetOnScreen only runs from
	// TickComponent, and USceneComponent::SetVisibility doesn't reliably reactivate that tick.
	BubbleWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UBubbleMessageComponent::Hide()
{
	if (UUserWidget* InnerWidget = WidgetComponent ? WidgetComponent->GetWidget() : nullptr)
	{
		InnerWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UBubbleMessageComponent::IsShowing() const
{
	if (const UUserWidget* InnerWidget = WidgetComponent ? WidgetComponent->GetWidget() : nullptr)
	{
		const ESlateVisibility Visibility = InnerWidget->GetVisibility();
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	}

	return false;
}

UBubbleMessageWidget* UBubbleMessageComponent::GetBubbleWidget() const
{
	return WidgetComponent ? Cast<UBubbleMessageWidget>(WidgetComponent->GetWidget()) : nullptr;
}
