// Copyright 2022 Mickael Daniel. All Rights Reserved.

#include "Graph/Slate/SComboGraphNode.h"

#include "ComboGraphEditorLog.h"
#include "SCommentBubble.h"
#include "SGraphPin.h"
#include "GraphEditorSettings.h"
#include "Graph/ComboGraphNodeBase.h"
#include "Graph/ComboGraphDragDropAction.h"
#include "Graph/Nodes/ComboGraphEdNode.h"
#include "Graph/Slate/SComboGraphNodeRow.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "GameplayEffect.h"
#include "Graph/ComboGraphNodeAnimBase.h"
#include "ComboGraphEditorTypes.h"
#include "Graph/ComboGraphNodeMontage.h"
#include "Graph/ComboGraphNodeSequence.h"

#define LOCTEXT_NAMESPACE "SComboGraphNode"

//////////////////////////////////////////////////////////////////////////
///
class SComboGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SComboGraphPin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin)
	{
		this->SetCursor(EMouseCursor::Default);

		bShowLabel = true;

		GraphPinObj = InPin;
		check(GraphPinObj != nullptr);

		const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
		check(Schema);

		SBorder::Construct(SBorder::FArguments()
			.BorderImage(this, &SComboGraphPin::GetPinBorder)
			.BorderBackgroundColor(this, &SComboGraphPin::GetPinColor)
			.OnMouseButtonDown(this, &SComboGraphPin::OnPinMouseDown)
			.Cursor(this, &SComboGraphPin::GetPinCursor)
			.Padding(FMargin(10.f))
		);
	}

protected:
	virtual FSlateColor GetPinColor() const override
	{
		constexpr FLinearColor Default(0.02f, 0.02f, 0.02f);
		// return Default;

		return FSlateColor(IsHovered() ? FLinearColor::Yellow : Default);
	}

	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override
	{
		return SNew(STextBlock);
	}

	const FSlateBrush* GetPinBorder() const
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		return FAppStyle::GetBrush(TEXT("Graph.StateNode.Body"));
#else
		return FEditorStyle::GetBrush(TEXT("Graph.StateNode.Body"));
#endif
	}

	virtual TSharedRef<FDragDropOperation> SpawnPinDragEvent(const TSharedRef<class SGraphPanel>& InGraphPanel, const TArray<TSharedRef<SGraphPin>>& InStartingPins) override
	{
		TArray<FGraphPinHandle> PinHandles;
		PinHandles.Reserve(InStartingPins.Num());

		// Since the graph can be refreshed and pins can be reconstructed/replaced behind the scenes, the DragDropOperation holds onto FGraphPinHandles
		// instead of direct widgets/graph-pins
		for (const TSharedRef<SGraphPin>& PinWidget : InStartingPins)
		{
			PinHandles.Add(PinWidget->GetPinObj());
		}

		return FComboGraphDragDropAction::New(InGraphPanel, PinHandles);
	}

};


//////////////////////////////////////////////////////////////////////////
void SComboGraphNode::Construct(const FArguments& InArgs, UComboGraphEdNode* InNode)
{
	check(InNode)
	GraphNode = InNode;
	CachedGraphNode = InNode;
	ContentPadding = InArgs._ContentPadding;
	ContentInternalPadding = InArgs._ContentInternalPadding;
	PinSize = InArgs._PinSize;
	PinPadding = InArgs._PinPadding;
	bDrawVerticalPins = InArgs._DrawVerticalPins;
	bDrawHorizontalPins = InArgs._DrawHorizontalPins;
	UpdateGraphNode();
	SetCursor(EMouseCursor::CardinalCross);
	InNode->SlateNode = this;
}

void SComboGraphNode::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (CachedGraphNode)
	{
		CachedGraphNode->UpdateTime(InDeltaTime);
	}
}

void SComboGraphNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();
	InputPinBoxLeft.Reset();
	InputPinBoxTop.Reset();
	OutputPinBoxRight.Reset();
	OutputPinBoxBottom.Reset();
	InputPinBoxTopWrapper.Reset();
	InputPinBoxLeftWrapper.Reset();
	OutputPinBoxBottomWrapper.Reset();
	OutputPinBoxRightWrapper.Reset();

	LeftNodeBox = InputPinBoxLeftWrapper;
	RightNodeBox = OutputPinBoxRightWrapper;

	//
	//             ______________________
	//            |      TITLE AREA      |
	//            +-------+------+-------+
	//            | (>) L |      | R (>) |
	//            | (>) E |      | I (>) |
	//            | (>) F |      | G (>) |
	//            | (>) T |      | H (>) |
	//            |       |      | T (>) |
	//            |_______|______|_______|
	//

	TSharedPtr<SErrorText> ErrorText;
	constexpr FLinearColor TitleShadowColor(0.6f, 0.6f, 0.6f, 0.3f);

	const TSharedPtr<SVerticalBox> NodeContent = CreateNodeContent();

	ContentScale.Bind(this, &SGraphNode::GetContentScale);
	GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
#else
			// .BorderImage(FEditorStyle::GetBrush("Graph.StateNode.Body"))
#endif
			.Padding(0.f)
			.BorderBackgroundColor(this, &SComboGraphNode::GetBorderBackgroundColor)
			[
				SNew(SOverlay)

				// Pins and node details
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SHorizontalBox)

					// Input Pin Area (Left)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[

						SNew(SBox)
						.MinDesiredWidth(PinSize)
						[
							SAssignNew(InputPinBoxLeftWrapper, SVerticalBox)
							+SVerticalBox::Slot()
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Fill)
							.Padding(0.f, PinPadding)
							.FillHeight(1.f)
							[
								SAssignNew(InputPinBoxLeft, SHorizontalBox)
							]
						]

					]

					// Main Area
					+ SHorizontalBox::Slot()
					[

						SNew(SVerticalBox)

						// Input Pin Area (Top)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SVerticalBox)

							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.MinDesiredHeight(PinSize)
								[
									SAssignNew(InputPinBoxTopWrapper, SVerticalBox)
									+SVerticalBox::Slot()
									.HAlign(HAlign_Fill)
									.VAlign(VAlign_Fill)
									.Padding(PinPadding, 0.f)
									.FillHeight(1.f)
									[
										SAssignNew(InputPinBoxTop, SHorizontalBox)
									]
								]
							]
						]

						// STATE NAME AREA
						+ SVerticalBox::Slot()
						// .HAlign(HAlign_Center)
						// .VAlign(VAlign_Center)
						.Padding(FMargin(ContentPadding))
						[
							SAssignNew(NodeBody, SBorder)
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
							.BorderImage(FAppStyle::GetBrush("Graph.StateNode.ColorSpill"))
#else
							.BorderImage(FEditorStyle::GetBrush("Graph.StateNode.ColorSpill"))
#endif
							.BorderBackgroundColor(TitleShadowColor)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Visibility(EVisibility::SelfHitTestInvisible)
							.Padding(ContentInternalPadding)
							[
								SNew(SOverlay)
								+SOverlay::Slot()
								.HAlign(HAlign_Fill)
								.VAlign(VAlign_Fill)
								[
									SNew(SVerticalBox)

									// TODO: Consider using SLevelOfDetailBranchNode like BTNode do (Graph LOD kind of)

									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SHorizontalBox)

										// Icon
										+SHorizontalBox::Slot()
										.AutoWidth()
										.VAlign(VAlign_Center)
										[
											SNew(SImage)
											.Image(this, &SComboGraphNode::GetNameIcon)
										]

										// Node Content
										+ SHorizontalBox::Slot()
										.Padding(FMargin(4.f, 0.f, 4.f, 0.f))
										[
											NodeContent.ToSharedRef()
										]
									]
								]
							]
						]

						// Output Pin Area (Bottom)
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBox)
							.MinDesiredHeight(PinSize)
							[
								SAssignNew(OutputPinBoxBottomWrapper, SVerticalBox)
								+SVerticalBox::Slot()
								.HAlign(HAlign_Fill)
								.VAlign(VAlign_Fill)
								.Padding(PinPadding, 0.f)
								.FillHeight(1.f)
								[
									SAssignNew(OutputPinBoxBottom, SHorizontalBox)
								]
							]
						]
					]

					// Output Pin Area (Right)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[

						SNew(SBox)
						.MinDesiredWidth(PinSize)
						[

							SAssignNew(OutputPinBoxRightWrapper, SVerticalBox)
							+SVerticalBox::Slot()
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Fill)
							.Padding(0.f, PinPadding)
							.FillHeight(1.f)
							[
								SAssignNew(OutputPinBoxRight, SHorizontalBox)
							]
						]

					]
				]

				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Bottom)
				.Padding(0.f, 8.f, 0.f, 0.f)
				[
					SAssignNew(ErrorText, SErrorText)
					.BackgroundColor(this, &SComboGraphNode::GetErrorColor)
					.ToolTipText(this, &SComboGraphNode::GetErrorMsgToolTip)
				]
			]
		];

	// Create comment bubble
	TSharedPtr<SCommentBubble> CommentBubble;
	const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;

	SAssignNew(CommentBubble, SCommentBubble)
		.GraphNode(GraphNode)
		.Text(this, &SGraphNode::GetNodeComment)
		.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
		.ColorAndOpacity(CommentColor)
		.AllowPinning(true)
		.EnableTitleBarBubble(true)
		.EnableBubbleCtrls(true)
		.GraphLOD(this, &SGraphNode::GetCurrentLOD)
		.IsGraphNodeHovered(this, &SGraphNode::IsHovered);

	GetOrAddSlot(ENodeZone::TopCenter)
		.SlotOffset(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetOffset))
		.SlotSize(TAttribute<FVector2D>(CommentBubble.Get(), &SCommentBubble::GetSize))
		.AllowScaling(TAttribute<bool>(CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
		.VAlign(VAlign_Top)
		[
			CommentBubble.ToSharedRef()
		];

	ErrorReporting = ErrorText;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	ErrorColor = FAppStyle::GetColor("ErrorReporting.BackgroundColor");
#else
	ErrorColor = FEditorStyle::GetColor("ErrorReporting.BackgroundColor");
#endif
	ErrorReporting->SetError(ErrorMsg);

	CreatePinWidgets();
}

void SComboGraphNode::CreatePinWidgets()
{
	UComboGraphEdNode* StateNode = CastChecked<UComboGraphEdNode>(GraphNode);

	for (int32 PinIdx = 0; PinIdx < StateNode->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* MyPin = StateNode->Pins[PinIdx];
		if (!MyPin->bHidden)
		{
			TSharedPtr<SGraphPin> NewPin = SNew(SComboGraphPin, MyPin);

			AddPin(NewPin.ToSharedRef());
		}
	}
}

void SComboGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if (bAdvancedParameter)
	{
		PinToAdd->SetVisibility( TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced) );
	}

	const FName PinName = PinObj ? PinObj->PinName : NAME_None;

	if (PinToAdd->GetDirection() == EGPD_Input)
	{
		if (PinName == UComboGraphPinNames::PinName_InTop)
		{
			InputPinBoxTop->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillWidth(1.f)
			// .Padding(PinMinPadding, 0.f)
			[
				PinToAdd
			];

			if (!bDrawVerticalPins)
			{
				InputPinBoxTop->SetVisibility(EVisibility::Collapsed);
			}

			InputPins.Add(PinToAdd);

		}
		else if (PinName == UComboGraphPinNames::PinName_InLeft)
		{
			InputPinBoxLeft->AddSlot()
		   .HAlign(HAlign_Fill)
		   .VAlign(VAlign_Fill)
		   .FillWidth(1.f)
		   // .Padding(PinMinPadding, 0.f)
			[
				PinToAdd
			];

			if (!bDrawHorizontalPins)
			{
				InputPinBoxLeft->SetVisibility(EVisibility::Collapsed);
			}

			InputPins.Add(PinToAdd);
		}
	}
	else // Direction == EEdGraphPinDirection::EGPD_Output
	{
		if (PinName == UComboGraphPinNames::PinName_OutBottom)
		{
			OutputPinBoxBottom->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillWidth(1.f)
			[
				PinToAdd
			];

			if (!bDrawVerticalPins)
			{
				OutputPinBoxBottom->SetVisibility(EVisibility::Collapsed);
			}

			OutputPins.Add(PinToAdd);
		}
		else if (PinName == UComboGraphPinNames::PinName_OutRight)
		{
			OutputPinBoxRight->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillWidth(1.f)
			[
				PinToAdd
			];

			if (!bDrawHorizontalPins)
			{
				OutputPinBoxRight->SetVisibility(EVisibility::Collapsed);
			}

			OutputPins.Add(PinToAdd);
		}
	}
}

bool SComboGraphNode::IsNameReadOnly() const
{
	UComboGraphEdNode* EdNode_Node = Cast<UComboGraphEdNode>(GraphNode);
	check(EdNode_Node != nullptr);

	return (!EdNode_Node->RuntimeNode->IsNameEditable()) || SGraphNode::IsNameReadOnly();
}

void SComboGraphNode::OnNameTextCommited(const FText& InText, const ETextCommit::Type CommitInfo)
{
	SGraphNode::OnNameTextCommited(InText, CommitInfo);

	UComboGraphEdNode* MyNode = CastChecked<UComboGraphEdNode>(GraphNode);

	if (MyNode != nullptr && MyNode->RuntimeNode != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("ACEGraphEditorRenameNode", "ACE Graph Editor: Rename Node"));
		MyNode->Modify();
		MyNode->RuntimeNode->Modify();
		MyNode->RuntimeNode->SetNodeTitle(InText);
		UpdateGraphNode();
	}
}

FSlateColor SComboGraphNode::GetBorderBackgroundColor() const
{
	const UComboGraphEdNode* MyNode = CastChecked<UComboGraphEdNode>(GraphNode);
	return MyNode->GetBackgroundColor();
}

EVisibility SComboGraphNode::GetDragOverMarkerVisibility() const
{
	return EVisibility::Visible;
}

const FSlateBrush* SComboGraphNode::GetNameIcon() const
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	return FAppStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Icon"));
#else
	return FEditorStyle::GetBrush(TEXT("BTEditor.Graph.BTNode.Icon"));
#endif
}

void SComboGraphNode::GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const
{
	if (!CachedGraphNode)
	{
		return;
	}

	const UComboGraphNodeBase* DebuggedNode = CachedGraphNode->GetDebuggedNode();
	if (!DebuggedNode)
	{
		return;
	}

	if (CachedGraphNode->IsDebugActive())
	{
		const FString Message = FString::Printf(TEXT("Active for %.2f secs"), CachedGraphNode->GetDebugTotalTime());
		new (Popups) FGraphInformationPopupInfo(nullptr, CachedGraphNode->GetBackgroundColor(), Message);
	}
	else if (CachedGraphNode->WasDebugActive())
	{
		const FString Message = FString::Printf(TEXT("Was Active for %.2f secs"), CachedGraphNode->GetDebugElapsedTime());
		new (Popups) FGraphInformationPopupInfo(nullptr, CachedGraphNode->GetBackgroundColor(), Message);
	}
}

// ReSharper disable once CppMemberFunctionMayBeConst
void SComboGraphNode::SetErrorText(const FText InErrorText)
{
	if (ErrorReporting)
	{
		ErrorReporting->SetError(InErrorText);
	}
}

TSharedPtr<SVerticalBox> SComboGraphNode::CreateNodeContent()
{
	TSharedPtr<SVerticalBox> Content = SNew(SVerticalBox);
	const TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	Content->AddSlot()
	.AutoHeight()
	[
		SAssignNew(InlineEditableText, SInlineEditableTextBlock)
		// .Style(FEditorStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
		.Style(FAppStyle::Get(), "Graph.StateNode.NodeTitleInlineEditableText")
		.Text(NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
		.OnVerifyTextChanged(this, &SComboGraphNode::OnVerifyNameTextChanged)
		.OnTextCommitted(this, &SComboGraphNode::OnNameTextCommited)
		.IsReadOnly(this, &SComboGraphNode::IsNameReadOnly)
		.IsSelected(this, &SComboGraphNode::IsSelectedExclusively)
	];

	Content->AddSlot()
	.AutoHeight()
	[
		NodeTitle.ToSharedRef()
	];

	if (!CachedGraphNode || !CachedGraphNode->RuntimeNode)
	{
		return Content;
	}

	const FMargin RowPadding = FMargin(0.f, 0.f, 0.f, 8.f);
	const FMargin TopRowPadding = FMargin(0.f, 8.f, 0.f, 8.f);
	const FMargin BottomRowPadding = FMargin(0.f, 0.f, 0.f, 0.f);

	Content->AddSlot()
	.Padding(TopRowPadding)
	.AutoHeight()
	[
		SNew(SComboGraphNodeRow, CachedGraphNode->RuntimeNode)
		.IsTransparent(false)
		.LabelText(CachedGraphNode->RuntimeNode->GetAnimAssetLabel())
		.LabelTooltipText(CachedGraphNode->RuntimeNode->GetAnimAssetLabelTooltip())
		.ValueTooltipText(CachedGraphNode->RuntimeNode->GetAnimAssetLabelTooltip())
		.OnGetValueText(FACEOnGetValueText::CreateSP(this, &SComboGraphNode::GetMontageText))
	];

	Content->AddSlot()
	.Padding(RowPadding)
	.AutoHeight()
	[
		SNew(SComboGraphNodeRow, CachedGraphNode->RuntimeNode)
		.IsTransparent(false)
		.LabelText(LOCTEXT("PlayRate", "Rate"))
		.LabelTooltipText(LOCTEXT("PlayRateTooltip", "Montage Play Rate"))
		.ValueTooltipText(LOCTEXT("PlayRateValueTooltip", "Montage Play Rate"))
		.OnGetValueText(FACEOnGetValueText::CreateSP(this, &SComboGraphNode::GetMontagePlayRateText))
	];

	if (CachedGraphNode->RuntimeNode)
	{
		Content->AddSlot()
		.Padding(RowPadding)
		.AutoHeight()
		[
			SNew(SComboGraphNodeRow, CachedGraphNode->RuntimeNode)
			.IsTransparent(false)
			.LabelText(LOCTEXT("StartSection", "Montage Start Section"))
			.LabelTooltipText(LOCTEXT("StartSection", "Montage Start Section"))
			.ValueTooltipText(LOCTEXT("StartSection", "Montage Start Section"))
			.OnGetValueText(FACEOnGetValueText::CreateSP(this, &SComboGraphNode::GetMontageStartSectionName))
			.Visibility(this, &SComboGraphNode::GetMontageStartSectionNameVisibility)
		];
	}

	Content->AddSlot()
	.Padding(RowPadding)
	.AutoHeight()
	[
		SNew(SComboGraphNodeRow, CachedGraphNode->RuntimeNode)
		.IsTransparent(false)
		.LabelText(LOCTEXT("EffectCost", "Effect Cost"))
		.LabelTooltipText(LOCTEXT("EffectCost", "Effect Cost"))
		.ValueTooltipText(LOCTEXT("EffectCost", "Effect Cost"))
		.OnGetValueText(FACEOnGetValueText::CreateSP(this, &SComboGraphNode::GetGameplayEffectCostText))
		.Visibility(this, &SComboGraphNode::GetGameplayEffectCostTextVisibility)
	];

	if (CachedGraphNode->RuntimeNode->IsComboSubclassedInBlueprint())
	{
		FString Classname = GetNameSafe(CachedGraphNode->RuntimeNode->GetClass());
		Classname.RemoveFromEnd("_C");

		Content->AddSlot()
		.Padding(BottomRowPadding)
		.AutoHeight()
		[
			SNew(SComboGraphNodeRow, CachedGraphNode->RuntimeNode)
			.IsTransparent(true)
			// .LabelText(LOCTEXT("NodeClass", "Node Class"))
			.LabelTooltipText(LOCTEXT("NodeClass", "Node Class"))
			.ValueTooltipText(LOCTEXT("NodeClass", "Node Class"))
			.OnGetValueText(FACEOnGetValueText::CreateLambda([Classname]()
			{
				return FText::FromString(*Classname);
			}))
		];
	}

	return Content;
}

EVisibility SComboGraphNode::GetGameplayEffectCostTextVisibility() const
{
	const TSubclassOf<UGameplayEffect> CostGE = CachedGraphNode->RuntimeNode->CostGameplayEffect;
	return IsValid(CostGE) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SComboGraphNode::GetGameplayEffectCostText() const
{
	const TSubclassOf<UGameplayEffect> CostGE = CachedGraphNode->RuntimeNode->CostGameplayEffect;
	return FText::FromString(CostGE ? CostGE->GetName() : TEXT("NONE"));
}

FText SComboGraphNode::GetMontagePlayRateText() const
{
	return FText::FromString(FString::SanitizeFloat(CachedGraphNode->RuntimeNode->MontagePlayRate));
}

FText SComboGraphNode::GetMontageText() const
{
	if (!CachedGraphNode || !CachedGraphNode->RuntimeNode)
	{
		return FText::FromString("");
	}

	return CachedGraphNode->RuntimeNode->GetAnimAssetText();
}

FText SComboGraphNode::GetMontageStartSectionName() const
{
	if (!CachedGraphNode || !CachedGraphNode->RuntimeNode)
	{
		return FText::FromName(NAME_None);
	}

	const FName StartSectionName = CachedGraphNode->RuntimeNode->GetAnimationStartSection();
	return FText::FromName(StartSectionName);
}

EVisibility SComboGraphNode::GetMontageStartSectionNameVisibility() const
{
	if (!CachedGraphNode || !CachedGraphNode->RuntimeNode)
	{
		return EVisibility::Collapsed;
	}

	const FName StartSectionName = CachedGraphNode->RuntimeNode->GetAnimationStartSection();
	return StartSectionName != NAME_None ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SComboGraphNode::GetErrorText() const
{
	return LOCTEXT("ErrorText", "Test Error Yo");
}


#undef LOCTEXT_NAMESPACE
