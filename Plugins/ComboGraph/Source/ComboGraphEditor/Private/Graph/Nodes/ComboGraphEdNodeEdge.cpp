// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/Nodes/ComboGraphEdNodeEdge.h"

#include "ComboGraphEditorTypes.h"
#include "ComboGraphEditorLog.h"
#include "InputMappingContext.h"
#include "Graph/ComboGraph.h"
#include "Graph/ComboGraphEdge.h"
#include "Graph/Nodes/ComboGraphEdNode.h"
#include "Graph/Nodes/ComboGraphEdNodeConduit.h"
#include "Utils/ComboGraphUtils.h"

UComboGraphEdNodeEdge::UComboGraphEdNodeEdge()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(this, &UComboGraphEdNodeEdge::OnObjectPropertyChanged);
}

UComboGraphEdNodeEdge::~UComboGraphEdNodeEdge()
{
    FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

void UComboGraphEdNodeEdge::SetRuntimeEdge(UComboGraphEdge* InEdge)
{
	RuntimeEdge = InEdge;
	UpdateCachedIcon();
}

void UComboGraphEdNodeEdge::CreateConnections(const UComboGraphEdNode* Start, const UComboGraphEdNode* End)
{
	CreateBaseConnections(Start, End);
}

void UComboGraphEdNodeEdge::CreateConnections(const UComboGraphEdNodeBase* Start, const UComboGraphEdNodeBase* End)
{
	CreateBaseConnections(Start, End);
}

void UComboGraphEdNodeEdge::CreateBaseConnections(const UComboGraphEdNodeBase* Start, const UComboGraphEdNodeBase* End)
{
	Pins[0]->Modify();
	Pins[0]->LinkedTo.Empty();

	Start->GetOutputPin()->Modify();
	Pins[0]->MakeLinkTo(Start->GetOutputPin());

	// This to next
	Pins[1]->Modify();
	Pins[1]->LinkedTo.Empty();

	End->GetInputPin()->Modify();
	Pins[1]->MakeLinkTo(End->GetInputPin());
}

UComboGraphEdNodeBase* UComboGraphEdNodeEdge::GetStartNodeAsBase() const
{
	if (Pins[0]->LinkedTo.Num() > 0)
	{
		return Cast<UComboGraphEdNodeBase>(Pins[0]->LinkedTo[0]->GetOwningNode());
	}

	return nullptr;
}

UComboGraphEdNode* UComboGraphEdNodeEdge::GetStartNodeAsGraphNode() const
{
	if (Pins[0]->LinkedTo.Num() > 0)
	{
		return Cast<UComboGraphEdNode>(Pins[0]->LinkedTo[0]->GetOwningNode());
	}

	return nullptr;
}

UComboGraphEdNodeConduit* UComboGraphEdNodeEdge::GetStartNodeAsConduit() const
{
	if (Pins[0]->LinkedTo.Num() > 0)
	{
		return Cast<UComboGraphEdNodeConduit>(Pins[0]->LinkedTo[0]->GetOwningNode());
	}

	return nullptr;
}

UComboGraphEdNode* UComboGraphEdNodeEdge::GetEndNode() const
{
	if (Pins[1]->LinkedTo.Num() > 0)
	{
		return Cast<UComboGraphEdNode>(Pins[1]->LinkedTo[0]->GetOwningNode());
	}

	return nullptr;
}

void UComboGraphEdNodeEdge::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, UComboGraphPinNames::PinCategory_Edge, FName(), TEXT("In"));
	Inputs->bHidden = true;
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, UComboGraphPinNames::PinCategory_Edge, FName(), TEXT("Out"));
	Outputs->bHidden = true;
}

void UComboGraphEdNodeEdge::PrepareForCopying()
{
	RuntimeEdge->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

FText UComboGraphEdNodeEdge::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (RuntimeEdge)
	{
		return RuntimeEdge->GetNodeTitle();
	}
	return FText();
}

void UComboGraphEdNodeEdge::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin->LinkedTo.Num() == 0)
	{
		// Transitions must always have an input and output connection
		Modify();

		// Our parent graph will have our graph in SubGraphs so needs to be modified to record that.
		if (UEdGraph* ParentGraph = GetGraph())
		{
			ParentGraph->Modify();
		}

		DestroyNode();
	}
}

void UComboGraphEdNodeEdge::UpdateCachedIcon()
{
	if (!RuntimeEdge)
	{
		CG_EDITOR_LOG(Error, TEXT("UComboGraphEdNodeEdge::UpdateCachedIcon - Invalid Runtime Edge"))
		CachedIcon = nullptr;
		return;
	}

	UInputAction* TransitionInput = RuntimeEdge->TransitionInput;
	if (!TransitionInput)
	{
		CachedIcon = nullptr;
		return;
	}

	if (!ComboGraphOwner)
	{
		CG_EDITOR_LOG(Verbose, TEXT("UComboGraphEdNodeEdge::UpdateCachedIcon - Set owner: %s"), *GetNameSafe(RuntimeEdge->GetOuter()))
		ComboGraphOwner = Cast<UComboGraph>(RuntimeEdge->GetOuter());
	}

	if (!ComboGraphOwner)
	{
		CG_EDITOR_LOG(Error, TEXT("UComboGraphEdNodeEdge::UpdateCachedIcon - ComboGraphOwner invalid, shouldn't happen (RuntimeEdge: %s)"), *GetNameSafe(RuntimeEdge->GetOuter()))
		return;
	}

	const UComboGraphProjectSettings* Settings = FComboGraphUtils::GetPluginProjectSettings();
	const TSoftObjectPtr<UDataTable> SoftDataTable = Settings->IconsDataTable;

	const TSoftObjectPtr<UInputMappingContext> SoftMappingContext = ComboGraphOwner->ContextMapping.IsNull() ? Settings->ContextMapping : ComboGraphOwner->ContextMapping;
	EComboGraphIconType IconPreference = ComboGraphOwner->IconPreference;

	// Build up the enum value string (useful later on to build row name str)
	FString IconStr = UEnum::GetValueAsString(IconPreference).Replace(TEXT("EComboGraphIconType::"), TEXT(""));

	if (SoftMappingContext.IsNull() || SoftDataTable.IsNull())
	{
		CachedIcon = nullptr;
		return;
	}

	// TODO: Consider putting mapping / icon lookup logic into FComboGraphUtils (or separate module / plugin ?)
	const UInputMappingContext* MappingContext = SoftMappingContext.LoadSynchronous();
	UDataTable* DataTable = SoftDataTable.LoadSynchronous();
	if (!MappingContext || !DataTable)
	{
		CachedIcon = nullptr;
		return;
	}

	const TArray<FEnhancedActionKeyMapping> Mappings = MappingContext->GetMappings();

	TArray<FEnhancedActionKeyMapping> KeyMappings = Mappings.FilterByPredicate([&TransitionInput](const FEnhancedActionKeyMapping& Other)
	{
		return Other.Action == TransitionInput;
	});

	const FString Context = FString(TEXT("UComboGraphEdNodeEdge::LookupInputsTable"));
	TSoftObjectPtr<UTexture2D> MatchingIcon = nullptr;

	for (FEnhancedActionKeyMapping KeyMapping : KeyMappings)
	{
		FKey Key = KeyMapping.Key;
		const bool bIsGamepadKey = Key.IsGamepadKey();

		const FString RowNameStr = FString::Printf(TEXT("%s.%s"), *IconStr, *Key.ToString());
		FComboGraphInputsMetaData* InputMetaData = DataTable->FindRow<FComboGraphInputsMetaData>(FName(*RowNameStr), Context, false);
		if (!InputMetaData)
		{
			continue;
		}

		UTexture2D* Icon = InputMetaData->Icon.LoadSynchronous();
		if (!Icon)
		{
			continue;
		}

		if ((IconPreference == EComboGraphIconType::Keyboard && !bIsGamepadKey) || (IconPreference != EComboGraphIconType::Keyboard && bIsGamepadKey))
		{
			MatchingIcon = InputMetaData->Icon;
			break;
		}
	}

	CachedIcon = MatchingIcon;
}

UTexture2D* UComboGraphEdNodeEdge::GetCachedIconTexture() const
{
	return !CachedIcon.IsNull() ? CachedIcon.LoadSynchronous() : nullptr;
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UComboGraphEdNodeEdge::OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
{
	const FName MemberPropertyName = (Event.MemberProperty != nullptr) ? Event.MemberProperty->GetFName() : NAME_None;
	const FName PropertyName = (Event.Property != nullptr) ? Event.Property->GetFName() : NAME_None;

	if (Object == RuntimeEdge && PropertyName == GET_MEMBER_NAME_CHECKED(UComboGraphEdge, TransitionInput))
	{
		UpdateCachedIcon();
	}
}
