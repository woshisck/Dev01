// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/Nodes/ComboGraphEdNode.h"

#include "ComboGraphEditorTypes.h"
#include "Abilities/Tasks/ComboGraphAbilityTask_StartGraph.h"
#include "Graph/ComboGraphEdGraph.h"
#include "Graph/ComboGraphNodeBase.h"
#include "Graph/ComboGraphNodeSequence.h"
#include "Graph/ComboGraphNodeAnimBase.h"
#include "Graph/Nodes/ComboGraphEdNodeEdge.h"
#include "Graph/Slate/SComboGraphNode.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Settings/ComboGraphProjectSettings.h"
#include "Utils/ComboGraphUtils.h"
#include "AssetEditor/ComboGraphDebugger.h"

UComboGraphEdNode::UComboGraphEdNode()
{
	bCanRenameNode = false;
}

FLinearColor UComboGraphEdNode::GetBackgroundColor() const
{
	if (!RuntimeNode)
	{
		return FLinearColor::Black;
	}

	const UComboGraphProjectSettings* Settings = FComboGraphUtils::GetPluginProjectSettings();
	const FLinearColor DebugActiveColor = Settings->DebugActiveColor;
	const FLinearColor DefaultColor = RuntimeNode->GetBackgroundColor();

	if (IsDebugActive())
	{
		return DebugActiveColor;
	}

	// Failsafe check to disable divide by 0
	const float DebugFadeTime = Settings->DebugFadeTime > 0 ? Settings->DebugFadeTime : 1.f;
	const float ActiveTime = WasActiveTime();
	if (WasDebugActive() && ActiveTime < DebugFadeTime)
	{
		return FLinearColor::LerpUsingHSV(DebugActiveColor, DefaultColor, ActiveTime / DebugFadeTime);
	}

	return DefaultColor;
}

void UComboGraphEdNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UComboGraphPinNames::PinCategory_MultipleNodes, FName(), UComboGraphPinNames::PinName_InLeft);
	CreatePin(EGPD_Input, UComboGraphPinNames::PinCategory_MultipleNodes, FName(), UComboGraphPinNames::PinName_InTop);
	CreatePin(EGPD_Output, UComboGraphPinNames::PinCategory_MultipleNodes, FName(), UComboGraphPinNames::PinName_OutRight);
	CreatePin(EGPD_Output, UComboGraphPinNames::PinCategory_MultipleNodes, FName(), UComboGraphPinNames::PinName_OutBottom);
}

void UComboGraphEdNode::PrepareForCopying()
{
	RuntimeNode->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

FText UComboGraphEdNode::GetNodeTitle(const ENodeTitleType::Type TitleType) const
{
	if (!RuntimeNode)
	{
		return Super::GetNodeTitle(TitleType);
	}

	return RuntimeNode->GetNodeTitle();
}

void UComboGraphEdNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin)
	{
		if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}

void UComboGraphEdNode::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	// TODO: Move most of the editor stuff into Developer module (or UncookedOnly). See how ControlRig is done regarding this
	// Super::ValidateNodeDuringCompilation(MessageLog);

	const bool bIsSequence = RuntimeNode->IsA(UComboGraphNodeSequence::StaticClass());

	if (bIsSequence)
	{
		MessageLog.Warning(TEXT("@@ Using sequences combo nodes is not supported in networked environment. Try using Montages instead."), this);
	}
}

void UComboGraphEdNode::PostEditUndo()
{
	Super::PostEditUndo();
}

void UComboGraphEdNode::UpdateTime(const float DeltaTime)
{
	if (!RuntimeNode)
	{
		bIsDebugActive = bWasDebugActive = false;
		return;
	}

	const UComboGraphNodeBase* DebuggedNode = GetDebuggedNode();
	if (!DebuggedNode)
	{
		bIsDebugActive = bWasDebugActive = false;
		DebugTotalTime = DebugElapsedTime = 0.f;
		return;
	}

	constexpr float DebugMaxTime = 2.f;

	// Debugged node changed, mark as was active if we were active
	if (DebuggedNode != RuntimeNode)
	{
		UpdateTimeRuntimeNodeChanged(DeltaTime, DebugMaxTime);
		return;
	}

	// Current debugged node is active
	if (DebuggedNode->bDebugActive)
	{
		UpdateTimeCurrentNode(DeltaTime, DebugMaxTime);
	}
	else
	{
		bIsDebugActive = bWasDebugActive = false;
		DebugTotalTime = DebugElapsedTime = 0.f;
	}
}

void UComboGraphEdNode::UpdateErrorReporting(USkeletalMesh* InSkeletalMesh, const FText ErrorMessage) const
{
	if (!SlateNode || !RuntimeNode)
	{
		return;
	}

	// check(SlateNode)
	// check(RuntimeNode)
	// check(InSkeletalMesh)

	if (!InSkeletalMesh)
	{
		// Passed in Skeletal Mesh is set to null, clear up if there was any
		SlateNode->SetErrorText(FText::FromString(""));
		return;
	}

	UAnimationAsset* AnimationAsset = RuntimeNode->GetAnimationAsset();
	if (!AnimationAsset)
	{
		// No Anim Asset for this node, clear up error if there was any
		SlateNode->SetErrorText(FText::FromString(""));
		return;
	}

	USkeleton* AnimSkeleton = AnimationAsset->GetSkeleton();
	if (!AnimSkeleton)
	{
		// Possible ? Anim Asset has no valid skeleton
		SlateNode->SetErrorText(FText::FromString(""));
		return;
	}

	#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 26
	const USkeleton* MeshSkeleton = InSkeletalMesh ? InSkeletalMesh->Skeleton : nullptr;
	#else
	const USkeleton* MeshSkeleton = InSkeletalMesh ? InSkeletalMesh->GetSkeleton() : nullptr;
	#endif

	FText ErrorText;
	#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	if (!AnimSkeleton->IsCompatibleForEditor(MeshSkeleton))
	#else
	if (!AnimSkeleton->IsCompatible(MeshSkeleton))
	#endif
	{
		// Case of skeleton mismatch, set error text with passed in one
		ErrorText = ErrorMessage;
	}

	SlateNode->SetErrorText(ErrorText);
}

float UComboGraphEdNode::WasActiveTime() const
{
	return DebugTotalTime - DebugElapsedTime;
}

void UComboGraphEdNode::UpdateTimeRuntimeNodeChanged(const float DeltaTime, const float DebugMaxTime)
{
	// Debugged node changed, mark as was active if we were active
	if (IsDebugActive() && !WasDebugActive())
	{
		bWasDebugActive = true;
		bIsDebugActive = false;
		DebugElapsedTime = DebugTotalTime;
		// DebugTotalTime = 0.f;
	}
	else if (WasDebugActive() && WasActiveTime() >= DebugMaxTime)
	{
		bWasDebugActive = false;
	}

	// Only tick time if we were active and for the duration of max debug time
	if (WasDebugActive() && DebugElapsedTime != 0)
	{
		DebugTotalTime = DebugTotalTime + DeltaTime;
	}
	else
	{
		bIsDebugActive = bWasDebugActive = false;
		DebugTotalTime = DebugElapsedTime = 0.f;
	}
}

void UComboGraphEdNode::UpdateTimeCurrentNode(const float DeltaTime, const float DebugMaxTime)
{
	// Turn on debug active state if it wasn't already
	if (!IsDebugActive())
	{
		bWasDebugActive = false;
		bIsDebugActive = true;
		DebugTotalTime = 0.f;
	}
	// If the node is no longer active and marked as was active, reset bWasDebugActive when it has been on long enough
	else if (!IsDebugActive() && WasDebugActive() && DebugTotalTime >= DebugMaxTime)
	{
		bWasDebugActive = false;
	}
	// Update debug time if node is active and didn't reached max debug time
	else
	{
		DebugTotalTime = DebugTotalTime + DeltaTime;
	}
}

const UComboGraphNodeBase* UComboGraphEdNode::GetDebuggedNode() const
{
	UComboGraphEdGraph* ComboEdGraph = Cast<UComboGraphEdGraph>(GetOuter());

	if (ComboEdGraph && ComboEdGraph->Debugger.IsValid())
	{
		FComboGraphDebugger* Debugger = ComboEdGraph->Debugger.Get();
		if (Debugger && Debugger->IsDebuggerReady())
		{
			UComboGraphAbilityTask_StartGraph* DebuggedTask = Debugger->GetDebuggedTaskForSelectedActor();
			if (DebuggedTask)
			{
				return DebuggedTask->GetCurrentNode();
			}
		}
	}

	return nullptr;
}
