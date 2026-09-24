#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/UserWidget.h"
#include "Character/YogPlayerControllerBase.h"
#include "Component/InteractPromptComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UI/InteractPromptWidget.h"
#include "UI/RuneRewardFloatWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractPromptComponentVisibilityTest,
	"DevKit.UI.InteractPrompt.ComponentTogglesInnerWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractPromptComponentVisibilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	AActor* Owner = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Owner actor spawned"), Owner);
	if (!Owner)
	{
		return false;
	}

	Owner->SetRootComponent(NewObject<USceneComponent>(Owner, TEXT("Root")));
	Owner->GetRootComponent()->RegisterComponent();

	UInteractPromptComponent* Prompt =
		NewObject<UInteractPromptComponent>(Owner, TEXT("InteractPromptComp"));
	Prompt->RegisterComponent();

	// The widget component has to belong to the actor, otherwise it never registers and never draws.
	UWidgetComponent* WidgetComponent = Prompt->GetWidgetComponent();
	TestNotNull(TEXT("Prompt spawned its widget component"), WidgetComponent);
	if (!WidgetComponent)
	{
		return false;
	}
	TestEqual(TEXT("Widget component is owned by the actor"), WidgetComponent->GetOwner(), Owner);
	TestTrue(TEXT("Widget component is registered"), WidgetComponent->IsRegistered());

	TestFalse(TEXT("Prompt starts hidden"), Prompt->IsPromptShowing());

	Prompt->SetPromptVisible(true);
	TestTrue(TEXT("Prompt shows after SetPromptVisible(true)"), Prompt->IsPromptShowing());
	if (const UUserWidget* InnerWidget = WidgetComponent->GetWidget())
	{
		// Hiding must happen on the inner widget: toggling the component is racy for screen space.
		TestTrue(TEXT("Component itself stays visible"), WidgetComponent->IsVisible());
		TestNotEqual(TEXT("Inner widget is not collapsed while showing"),
			InnerWidget->GetVisibility(), ESlateVisibility::Collapsed);
	}

	Prompt->SetPromptVisible(false);
	TestFalse(TEXT("Prompt hides again"), Prompt->IsPromptShowing());

	Owner->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRuneRewardFloatPromptPulseScaleTest,
	"DevKit.UI.RuneRewardFloatWidget.PromptPulseScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRuneRewardFloatPromptPulseScaleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(
		TEXT("Prompt pulse starts above base scale"),
		URuneRewardFloatWidget::ComputePromptHighlightScale(0.f, 2.f) > 1.f);

	TestEqual(
		TEXT("Prompt pulse returns to base scale at the end"),
		URuneRewardFloatWidget::ComputePromptHighlightScale(2.f, 2.f),
		1.f);

	TestEqual(
		TEXT("Prompt pulse clamps elapsed time beyond duration"),
		URuneRewardFloatWidget::ComputePromptHighlightScale(4.f, 2.f),
		1.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInteractHoldProgressTest,
	"DevKit.Interact.HoldProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInteractHoldProgressTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(
		TEXT("Hold starts empty"),
		AYogPlayerControllerBase::ComputeHoldProgress(0.f, 0.6f),
		0.f);

	TestEqual(
		TEXT("Half the duration fills half the bar"),
		AYogPlayerControllerBase::ComputeHoldProgress(0.3f, 0.6f),
		0.5f);

	TestEqual(
		TEXT("Reaching the duration completes the hold"),
		AYogPlayerControllerBase::ComputeHoldProgress(0.6f, 0.6f),
		1.f);

	TestEqual(
		TEXT("Overshooting the duration clamps to full"),
		AYogPlayerControllerBase::ComputeHoldProgress(5.f, 0.6f),
		1.f);

	// A zero duration is how a target opts back into instant interaction.
	TestEqual(
		TEXT("Zero duration completes immediately"),
		AYogPlayerControllerBase::ComputeHoldProgress(0.f, 0.f),
		1.f);

	TestEqual(
		TEXT("Negative duration completes immediately"),
		AYogPlayerControllerBase::ComputeHoldProgress(0.f, -1.f),
		1.f);

	TestEqual(
		TEXT("Negative elapsed clamps to empty"),
		AYogPlayerControllerBase::ComputeHoldProgress(-1.f, 0.6f),
		0.f);

	return true;
}

#endif
