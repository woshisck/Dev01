#include "Component/InteractPromptComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "UI/InteractPromptWidget.h"

UInteractPromptComponent::UInteractPromptComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractPromptComponent::OnRegister()
{
	Super::OnRegister();

	EnsureWidgetComponent();
}

void UInteractPromptComponent::EnsureWidgetComponent()
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
	WidgetComponent = NewObject<UWidgetComponent>(Owner, TEXT("InteractPromptWidgetComp"));
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComponent->SetDrawAtDesiredSize(true);
	WidgetComponent->SetWidgetClass(UInteractPromptWidget::StaticClass());
	WidgetComponent->SetupAttachment(Owner->GetRootComponent());
	WidgetComponent->RegisterComponent();
	WidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, PromptHeight));

	// Registration makes UWidgetComponent build the widget itself, at the default (visible) Slate
	// visibility. Collapse it here or the prompt is on screen from spawn until the first overlap.
	EnsureWidget();
	if (UUserWidget* InnerWidget = WidgetComponent->GetWidget())
	{
		InnerWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInteractPromptComponent::EnsureWidget()
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

	WidgetComponent->SetWidgetClass(UInteractPromptWidget::StaticClass());
	WidgetComponent->InitWidget();
}

void UInteractPromptComponent::SetPromptVisible(bool bVisible)
{
	EnsureWidget();
	if (!WidgetComponent)
	{
		return;
	}

	// The component itself stays visible so it keeps getting added to the screen layer; only the
	// inner widget's Slate visibility is toggled. Toggling component visibility is racy on
	// screen-space widgets because UWidgetComponent::UpdateWidgetOnScreen only runs from
	// TickComponent, and USceneComponent::SetVisibility doesn't reliably reactivate that tick.
	if (UUserWidget* InnerWidget = WidgetComponent->GetWidget())
	{
		InnerWidget->SetVisibility(bVisible
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (!bVisible)
	{
		SetHoldProgress(0.f);
	}
}

bool UInteractPromptComponent::IsPromptShowing() const
{
	if (const UUserWidget* InnerWidget = WidgetComponent ? WidgetComponent->GetWidget() : nullptr)
	{
		const ESlateVisibility Visibility = InnerWidget->GetVisibility();
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	}

	return false;
}

void UInteractPromptComponent::SetHoldProgress(float Normalized)
{
	if (UInteractPromptWidget* PromptWidget = GetPromptWidget())
	{
		PromptWidget->SetHoldProgress(Normalized);
	}
}

UInteractPromptWidget* UInteractPromptComponent::GetPromptWidget() const
{
	return WidgetComponent ? Cast<UInteractPromptWidget>(WidgetComponent->GetWidget()) : nullptr;
}
