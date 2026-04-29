// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Graph/Slate/SComboGraphNodeConduit.h"

#include "IDocumentation.h"
#include "Graph/Nodes/ComboGraphEdNodeConduit.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Graph/Slate/SComboGraphConduitOutputPin.h"

#define LOCTEXT_NAMESPACE "SComboGraphNodeConduit"

void SComboGraphNodeConduit::Construct(const FArguments& InArgs, UComboGraphEdNodeConduit* InNode)
{
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SComboGraphNodeConduit::GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& GraphInformationPopupInfos) const
{
	// Intentionally empty (might consider adding debug states here)
}

void SComboGraphNodeConduit::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	const FSlateBrush* NodeTypeIcon = GetNameIcon();

	constexpr FLinearColor TitleShadowColor(0.6f, 0.6f, 0.6f);
	TSharedPtr<SErrorText> ErrorText;
	const TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	ContentScale.Bind(this, &SGraphNode::GetContentScale);

	GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
#else
			.BorderImage(FEditorStyle::GetBrush("Graph.StateNode.Body"))
#endif
			.Padding(0)
			.BorderBackgroundColor(this, &SComboGraphNodeConduit::GetBorderBackgroundColor)
			[
				SNew(SOverlay)

				// PIN AREA
				+SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SAssignNew(RightNodeBox, SVerticalBox)
				]

				// STATE NAME AREA
				+SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(10.0f)
				[
					SNew(SBorder)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
					.BorderImage( FAppStyle::GetBrush("Graph.StateNode.ColorSpill") )
#else
					.BorderImage( FEditorStyle::GetBrush("Graph.StateNode.ColorSpill") )
#endif
					.BorderBackgroundColor( TitleShadowColor )
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Visibility(EVisibility::SelfHitTestInvisible)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							// POPUP ERROR MESSAGE
							SAssignNew(ErrorText, SErrorText)
							.BackgroundColor(this, &SComboGraphNodeConduit::GetErrorColor)
							.ToolTipText(this, &SComboGraphNodeConduit::GetErrorMsgToolTip)
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(NodeTypeIcon)
						]
						+SHorizontalBox::Slot()
						.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SAssignNew(InlineEditableText, SInlineEditableTextBlock)
								// .Style(FEditorStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
								.Style(FAppStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
								.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
								.OnVerifyTextChanged(this, &SComboGraphNodeConduit::OnVerifyNameTextChanged)
								.OnTextCommitted(this, &SComboGraphNodeConduit::OnNameTextCommited)
								.IsReadOnly(this, &SComboGraphNodeConduit::IsNameReadOnly)
								.IsSelected(this, &SComboGraphNodeConduit::IsSelectedExclusively)
							]
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								NodeTitle.ToSharedRef()
							]
						]
					]
				]
			]
		];

	ErrorReporting = ErrorText;
	ErrorReporting->SetError(ErrorMsg);
	CreatePinWidgets();
}

void SComboGraphNodeConduit::CreatePinWidgets()
{
	const UComboGraphEdNodeConduit* StateNode = CastChecked<UComboGraphEdNodeConduit>(GraphNode);

	UEdGraphPin* CurPin = StateNode->GetOutputPin();
	if (!CurPin->bHidden)
	{
		TSharedPtr<SGraphPin> NewPin = SNew(SComboGraphConduitOutputPin, CurPin);
		AddPin(NewPin.ToSharedRef());
	}
}

void SComboGraphNodeConduit::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));
	RightNodeBox->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.FillHeight(1.0f)
		[
			PinToAdd
		];
	OutputPins.Add(PinToAdd);
}

TSharedPtr<SToolTip> SComboGraphNodeConduit::GetComplexTooltip()
{
	UComboGraphEdNodeConduit* StateNode = CastChecked<UComboGraphEdNodeConduit>(GraphNode);

	return SNew(SToolTip)
		[
			SNew(SVerticalBox)

			// TODO: Look at this implementation for custom tooltip for montage / sequence nodes where we would render anim in a preview persona window,
			// alongside documentation and / or additional properties

			// +SVerticalBox::Slot()
			// .AutoHeight()
			// [
			// 	// Create the tooltip preview, ensure to disable state overlays to stop
			// 	// PIE and read-only borders obscuring the graph
			// 	SNew(SGraphPreviewer, StateNode->GetBoundGraph())
			// 	.CornerOverlayText(this, &SComboGraphNodeConduit::GetPreviewCornerText)
			// 	.ShowGraphStateOverlay(false)
			// ]

			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 5.0f, 0.0f, 0.0f))
			[
				IDocumentation::Get()->CreateToolTip(FText::FromString("Documentation"), nullptr, StateNode->GetDocumentationLink(), StateNode->GetDocumentationExcerptName())
			]

		];
}

FSlateColor SComboGraphNodeConduit::GetBorderBackgroundColor() const
{
	// Intentionally basic (might consider adding debug states here)
	constexpr FLinearColor InactiveStateColor(0.08f, 0.08f, 0.08f);
	FLinearColor ActiveStateColorDim(0.4f, 0.3f, 0.15f);
	FLinearColor ActiveStateColorBright(1.f, 0.6f, 0.35f);

	return InactiveStateColor;
}

const FSlateBrush* SComboGraphNodeConduit::GetNameIcon() const
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	return FAppStyle::GetBrush( TEXT("Graph.ConduitNode.Icon") );
#else
	return FEditorStyle::GetBrush( TEXT("Graph.ConduitNode.Icon") );
#endif
}

#undef LOCTEXT_NAMESPACE
