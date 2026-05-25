#include "LevelFlow/Nodes/LENode_GetStoryContext.h"

#include "FlowAsset.h"
#include "Story/Encounter/StoryFlowProxy.h"

namespace
{
const FName PinIn(TEXT("In"));
const FName PinOut(TEXT("Out"));
const FName PinSourceActor(TEXT("SourceActor"));
const FName PinContextTransform(TEXT("ContextTransform"));
const FName PinPlayerController(TEXT("PlayerController"));
}

ULENode_GetStoryContext::ULENode_GetStoryContext(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("LevelEvent|Story");
#endif
	InputPins = { FFlowPin(PinIn) };
	OutputPins = {
		FFlowPin(PinOut),
		FFlowPin(PinSourceActor, EFlowPinType::Object, AActor::StaticClass()),
		FFlowPin(PinContextTransform, EFlowPinType::Transform),
		FFlowPin(PinPlayerController, EFlowPinType::Object, APlayerController::StaticClass()),
	};
}

bool ULENode_GetStoryContext::CanSupplyDataPinValues_Implementation() const
{
	return true;
}

FFlowDataPinResult_Object ULENode_GetStoryContext::TrySupplyDataPinAsObject_Implementation(const FName& PinName) const
{
	const AStoryFlowProxy* Proxy = GetStoryFlowProxyOwner();

	if (PinName == PinSourceActor)
	{
		return FFlowDataPinResult_Object(Proxy ? Proxy->GetContextSourceActor() : nullptr);
	}

	if (PinName == PinPlayerController)
	{
		return FFlowDataPinResult_Object(Proxy ? Proxy->GetContextPlayerController() : nullptr);
	}

	return FFlowDataPinResult_Object();
}

FFlowDataPinResult_Transform ULENode_GetStoryContext::TrySupplyDataPinAsTransform_Implementation(const FName& PinName) const
{
	if (PinName == PinContextTransform)
	{
		const AStoryFlowProxy* Proxy = GetStoryFlowProxyOwner();
		return FFlowDataPinResult_Transform(Proxy ? Proxy->GetContextTransform() : FTransform::Identity);
	}

	return FFlowDataPinResult_Transform();
}

void ULENode_GetStoryContext::ExecuteInput(const FName& PinName)
{
	TriggerOutput(PinOut, true);
}

const AStoryFlowProxy* ULENode_GetStoryContext::GetStoryFlowProxyOwner() const
{
	return Cast<AStoryFlowProxy>(TryGetRootFlowActorOwner());
}
