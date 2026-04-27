#include "LevelFlow/Nodes/LENode_ShowTutorial.h"
#include "Tutorial/TutorialManager.h"

ULENode_ShowTutorial::ULENode_ShowTutorial(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("LevelEvent");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("OnClosed")) };
}

void ULENode_ShowTutorial::ExecuteInput(const FName& PinName)
{
	UTutorialManager* TM = GetTutorialManager();
	if (!TM)
	{
		TriggerOutput(TEXT("OnClosed"), true);
		return;
	}

	// 绑定弹窗关闭委托，关闭后触发 OnClosed
	PopupClosedHandle = TM->OnPopupClosed.AddLambda([this]()
	{
		TriggerOutput(TEXT("OnClosed"), true);
	});

	// InlinePages 优先；为空则查 DialogContentDA
	if (InlinePages.Num() > 0)
		TM->ShowInlinePages(InlinePages, GetPlayerController(), bPauseGame);
	else
		TM->ShowByEventID(EventID, GetPlayerController(), bPauseGame);
}

void ULENode_ShowTutorial::Cleanup()
{
	if (UTutorialManager* TM = GetTutorialManager())
	{
		TM->OnPopupClosed.Remove(PopupClosedHandle);
	}
	PopupClosedHandle.Reset();
	Super::Cleanup();
}
