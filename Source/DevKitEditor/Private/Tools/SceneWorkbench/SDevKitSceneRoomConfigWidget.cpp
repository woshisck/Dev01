#include "Tools/SceneWorkbench/SDevKitSceneRoomConfigWidget.h"
#include "Tools/Workflow/DevKitWorkbenchUI.h"

#include "Containers/Ticker.h"
#include "Data/RoomDataAsset.h"
#include "IDetailTreeNode.h"
#include "IPropertyRowGenerator.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitSceneRoomConfig"

namespace DevKitSceneRoomConfig
{
	const FLinearColor DefaultsAccent(0.35f, 0.67f, 0.92f);
	const FLinearColor GameplayAccent(0.9f, 0.65f, 0.32f);
	const FLinearColor RewardsAccent(0.44f, 0.76f, 0.59f);

	TSharedRef<SWidget> Note(const FText& Text)
	{
		return SNew(STextBlock).Text(Text).AutoWrapText(true)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.Font(FAppStyle::GetFontStyle("SmallFont"));
	}
}

void SDevKitSceneRoomConfigWidget::Construct(const FArguments& InArgs)
{
	DefaultsPanel = SNew(SBox);
	GameplayPanel = SNew(SBox);
	FPropertyRowGeneratorArgs Args;
	RowGenerator = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor")
		.CreatePropertyRowGenerator(Args);
	RowGenerator->OnRowsRefreshed().AddSP(this, &SDevKitSceneRoomConfigWidget::QueueRebuild);
	RowGenerator->OnFinishedChangingProperties().AddSP(this,
		&SDevKitSceneRoomConfigWidget::OnPropertiesChanged);
	// The explorer owns the returned panel widgets; this controller is retained by it.
	ChildSlot[SNullWidget::NullWidget];
	RebuildPanels();
}

void SDevKitSceneRoomConfigWidget::SetRoom(URoomDataAsset* InRoom)
{
	Room.Reset(InRoom);
	PropertyHandles.Reset();
	TArray<UObject*> Objects;
	if (InRoom)
	{
		Objects.Add(InRoom);
	}
	RowGenerator->SetObjects(Objects);
	RebuildPanels();
}

TSharedRef<SWidget> SDevKitSceneRoomConfigWidget::MakeDefaultsPanel()
{
	return DefaultsPanel.ToSharedRef();
}

TSharedRef<SWidget> SDevKitSceneRoomConfigWidget::MakeGameplayPanel()
{
	return GameplayPanel.ToSharedRef();
}

URoomDataAsset* SDevKitSceneRoomConfigWidget::GetRoom() const
{
	return Room.Get();
}

void SDevKitSceneRoomConfigWidget::GatherHandles(const TArray<TSharedRef<IDetailTreeNode>>& Nodes)
{
	for (const TSharedRef<IDetailTreeNode>& Node : Nodes)
	{
		const TSharedPtr<IPropertyHandle> Handle = Node->CreatePropertyHandle();
		const TSharedPtr<IPropertyHandle> Parent = Handle.IsValid() ? Handle->GetParentHandle() : nullptr;
		if (Handle.IsValid() && Handle->IsValidHandle() && Handle->GetProperty()
			&& Handle->GetProperty()->GetOwnerStruct() == URoomDataAsset::StaticClass()
			&& (!Parent.IsValid() || Parent->GetProperty() == nullptr))
		{
			// An array element's inner property has the SAME name as its parent array.
			// Do not replace top-level arrays with those struct/object element handles.
			PropertyHandles.Add(Handle->GetProperty()->GetFName(), Handle);
		}
		// Root categories include advanced groups. Keep the entire editable contract.
		TArray<TSharedRef<IDetailTreeNode>> Children;
		Node->GetChildren(Children, true);
		GatherHandles(Children);
	}
}

void SDevKitSceneRoomConfigWidget::RebuildPanels()
{
	bRebuildQueued = false;
	PropertyHandles.Reset();
	if (Room.IsValid())
	{
		GatherHandles(RowGenerator->GetRootTreeNodes());
	}
	DefaultsPanel->SetContent(Room.IsValid() ? BuildDefaults() : MakeEmptyState());
	GameplayPanel->SetContent(Room.IsValid() ? BuildGameplay() : MakeEmptyState());
}

void SDevKitSceneRoomConfigWidget::QueueRebuild()
{
	if (bRebuildQueued)
	{
		return;
	}
	bRebuildQueued = true;
	const TWeakPtr<SDevKitSceneRoomConfigWidget> WeakThis =
		StaticCastSharedRef<SDevKitSceneRoomConfigWidget>(AsShared());
	// Rebuild after the property generator has completed its array/undo refresh.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
	{
		if (const TSharedPtr<SDevKitSceneRoomConfigWidget> Pinned = WeakThis.Pin())
		{
			Pinned->RebuildPanels();
		}
		return false;
	}));
}

void SDevKitSceneRoomConfigWidget::OnPropertiesChanged(const FPropertyChangedEvent& Event)
{
	if (Room.IsValid())
	{
		Room->MarkPackageDirty();
	}
	// Native handles send transactions and PostEditChangeProperty; no parallel data copy.
}

TSharedPtr<IPropertyHandle> SDevKitSceneRoomConfigWidget::FindHandle(FName Name) const
{
	return PropertyHandles.FindRef(Name);
}

bool SDevKitSceneRoomConfigWidget::IsHandleEditable(TSharedPtr<IPropertyHandle> Handle) const
{
	if (!Room.IsValid() || !Handle.IsValid() || !Handle->IsValidHandle() || !Handle->IsEditable())
	{
		return false;
	}
	return true;
}

TSharedRef<SWidget> SDevKitSceneRoomConfigWidget::MakeValue(TSharedPtr<IPropertyHandle> Handle)
{
	if (!Handle.IsValid() || !Handle->IsValidHandle())
	{
		return DevKitSceneRoomConfig::Note(LOCTEXT("UnavailableField", "该字段暂不可用，请刷新资产。"));
	}
	return SNew(SBox).MinDesiredWidth(100.0f)
		.IsEnabled_Lambda([this, Handle] { return IsHandleEditable(Handle); })
		// Preserve native GameplayTag, FVector and other type customizations without
		// exposing the original asset's full DA details tree.
		[Handle->CreatePropertyValueWidgetWithCustomization(nullptr)];
}

TSharedRef<SWidget> SDevKitSceneRoomConfigWidget::MakeHandleField(TSharedPtr<IPropertyHandle> Handle,
	const FText& Label, const FText& Hint)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.36f).VAlign(VAlign_Center).Padding(0, 6, 12, 6)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Label)
				.ToolTipText(Handle.IsValid() ? Handle->GetToolTipText() : FText::GetEmpty())]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
			[DevKitSceneRoomConfig::Note(Hint)]
		]
		+ SHorizontalBox::Slot().FillWidth(0.64f).VAlign(VAlign_Center).Padding(0, 5)
		[MakeValue(Handle)];
}

TSharedRef<SWidget> SDevKitSceneRoomConfigWidget::MakeField(FName Name,
	const FText& Label, const FText& Hint)
{
	return MakeHandleField(FindHandle(Name), Label, Hint);
}

TSharedRef<SWidget> SDevKitSceneRoomConfigWidget::MakeCard(const FText& Title,
	const FText& Description, TSharedRef<SWidget> Content, FLinearColor Accent, bool bAdvanced)
{
	const FString CardKey = Title.ToString();
	const bool* SavedExpansion = ExpandedCards.Find(CardKey);
	const bool bExpanded = SavedExpansion ? *SavedExpansion : !bAdvanced;
	const TWeakPtr<SDevKitSceneRoomConfigWidget> WeakThis = SharedThis(this);
	return DevKitWorkbenchUI::MakeSection(Title, Content, !bExpanded, Description, Accent,
		FOnBooleanValueChanged::CreateLambda([WeakThis, CardKey](bool bNowExpanded)
		{
			if (const TSharedPtr<SDevKitSceneRoomConfigWidget> Self = WeakThis.Pin())
				Self->ExpandedCards.FindOrAdd(CardKey) = bNowExpanded;
		}));
}

TSharedRef<SWidget> SDevKitSceneRoomConfigWidget::MakeEmptyState() const
{
	return SNew(SBorder).Padding(20).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock)
				.Text(LOCTEXT("SelectRoom", "选择房间配置"))
				.Font(FAppStyle::GetFontStyle("NormalFontBold"))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
			[DevKitSceneRoomConfig::Note(LOCTEXT("SelectRoomHelp",
				"请先选择左侧关卡，再选择对应 RoomData。一个美术关卡可以关联多个玩法配置；不会自动合并或覆盖。"))]];
}

FReply SDevKitSceneRoomConfigWidget::AddArrayItem(TSharedPtr<IPropertyHandle> Handle)
{
	if (IsHandleEditable(Handle))
	{
		if (const TSharedPtr<IPropertyHandleArray> Array = Handle->AsArray())
		{
			Array->AddItem();
			QueueRebuild();
		}
	}
	return FReply::Handled();
}

FReply SDevKitSceneRoomConfigWidget::DeleteArrayItem(TSharedPtr<IPropertyHandle> Handle, int32 Index)
{
	if (IsHandleEditable(Handle))
	{
		if (const TSharedPtr<IPropertyHandleArray> Array = Handle->AsArray())
		{
			Array->DeleteItem(Index);
			QueueRebuild();
		}
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SDevKitSceneRoomConfigWidget::MakeArray(TSharedPtr<IPropertyHandle> Handle,
	const FText& EntryLabel, const TArray<FFieldColumn>& Columns)
{
	const TSharedPtr<IPropertyHandleArray> Array = Handle.IsValid() ? Handle->AsArray() : nullptr;
	if (!Array.IsValid())
	{
		return DevKitSceneRoomConfig::Note(LOCTEXT("UnavailableArray", "数组结构暂不可用，请刷新资产；不会用结构摘要替代可编辑条目。"));
	}
	Array->SetOnNumElementsChanged(FSimpleDelegate::CreateSP(this,
		&SDevKitSceneRoomConfigWidget::QueueRebuild));
	uint32 Count = 0;
	Array->GetNumElements(Count);
	const TSharedRef<SVerticalBox> Table = SNew(SVerticalBox);
	Table->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[DevKitSceneRoomConfig::Note(FText::Format(LOCTEXT("ArrayCount", "{0} · {1} 项"), EntryLabel, FText::AsNumber(Count)))]
			+ SHorizontalBox::Slot().AutoWidth()
			[SNew(SButton).Text(LOCTEXT("AddEntry", "+ 添加条目"))
				.IsEnabled_Lambda([this, Handle] { return IsHandleEditable(Handle); })
				.OnClicked(this, &SDevKitSceneRoomConfigWidget::AddArrayItem, Handle)]];
	if (!Columns.IsEmpty() && Columns.Num() <= 4 && Count > 0)
	{
		const TSharedRef<SHorizontalBox> Labels = SNew(SHorizontalBox);
		Labels->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(28)];
		for (const FFieldColumn& Column : Columns)
		{
			Labels->AddSlot().FillWidth(Column.Weight).Padding(4, 3)
				[DevKitSceneRoomConfig::Note(Column.Label)];
		}
		Labels->AddSlot().AutoWidth()[SNew(SBox).WidthOverride(52)];
		Table->AddSlot().AutoHeight()[Labels];
	}
	for (uint32 Index = 0; Index < Count; ++Index)
	{
		const TSharedRef<IPropertyHandle> Element = Array->GetElement(Index);
		if (Columns.Num() > 4)
		{
			// Wide reward structs become readable two-row cards instead of six cramped columns.
			const TSharedRef<SUniformGridPanel> EntryGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(4, 6));
			for (int32 FieldIndex = 0; FieldIndex < Columns.Num(); ++FieldIndex)
			{
				const FFieldColumn& Field = Columns[FieldIndex];
				EntryGrid->AddSlot(FieldIndex % 3, FieldIndex / 3)
					[SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
						[DevKitSceneRoomConfig::Note(Field.Label)]
						+ SVerticalBox::Slot().AutoHeight()[MakeValue(Element->GetChildHandle(Field.Name))]];
			}
			Table->AddSlot().AutoHeight().Padding(0, 3)
				[SNew(SBorder).BorderImage(FAppStyle::GetBrush("Brushes.Recessed")).Padding(8)
					[SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
							[SNew(STextBlock).Text(FText::Format(LOCTEXT("EntryNumber", "{0} {1}"), EntryLabel, FText::AsNumber(Index + 1)))
								.Font(FAppStyle::GetFontStyle("NormalFontBold"))]
							+ SHorizontalBox::Slot().AutoWidth()
							[SNew(SButton).Text(LOCTEXT("RemoveEntry", "删除条目"))
								.IsEnabled_Lambda([this, Handle] { return IsHandleEditable(Handle); })
								.OnClicked(this, &SDevKitSceneRoomConfigWidget::DeleteArrayItem, Handle, static_cast<int32>(Index))]]
						+ SVerticalBox::Slot().AutoHeight()[EntryGrid]]];
			continue;
		}
		const TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center)
			[SNew(SBox).WidthOverride(28)[SNew(STextBlock).Text(FText::AsNumber(Index + 1))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())]];
		if (Columns.IsEmpty())
		{
			Row->AddSlot().FillWidth(1).VAlign(VAlign_Center).Padding(4)[MakeValue(Element)];
		}
		else
		{
			for (const FFieldColumn& Column : Columns)
			{
				Row->AddSlot().FillWidth(Column.Weight).VAlign(VAlign_Center).Padding(4)
					[MakeValue(Element->GetChildHandle(Column.Name))];
			}
		}
		Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(4)
			[SNew(SButton).Text(LOCTEXT("RemoveEntry", "删除条目"))
				.ToolTipText(LOCTEXT("RemoveEntryHelp", "移除此配置条目，可通过编辑器撤销恢复。"))
				.IsEnabled_Lambda([this, Handle] { return IsHandleEditable(Handle); })
				.OnClicked(this, &SDevKitSceneRoomConfigWidget::DeleteArrayItem, Handle, static_cast<int32>(Index))];
		Table->AddSlot().AutoHeight().Padding(0, 1)
			[SNew(SBorder).BorderImage(FAppStyle::GetBrush("Brushes.Recessed")).Padding(5)[Row]];
	}
	if (Count == 0)
	{
		Table->AddSlot().AutoHeight().Padding(4, 6)
			[DevKitSceneRoomConfig::Note(LOCTEXT("NoArrayEntries", "尚未配置；点击添加新条目。"))];
	}
	return Table;
}

TSharedRef<SWidget> SDevKitSceneRoomConfigWidget::BuildDefaults()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[MakeCard(LOCTEXT("DefaultsTitle", "关卡默认设置"),
			LOCTEXT("DefaultsDescription", "原 RoomData 标识与类型；地图关联和运行时加载名称保持原有逻辑。"),
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[MakeField(TEXT("RoomName"), LOCTEXT("RoomName", "运行时关卡名称"),
					LOCTEXT("RoomNameHint", "程序按此名称加载地图；修改前请确认对应地图名称。"))]
				+ SVerticalBox::Slot().AutoHeight()[MakeField(TEXT("DisplayName"), LOCTEXT("DisplayName", "玩家可见名称"))]
				+ SVerticalBox::Slot().AutoHeight()[MakeField(TEXT("RoomTags"), LOCTEXT("RoomTags", "房间类型 / 层级标签"),
					LOCTEXT("RoomTagsHint", "类型与层级共同参与 Campaign 的房间筛选。"))],
			DevKitSceneRoomConfig::DefaultsAccent)];
}

TSharedRef<SWidget> SDevKitSceneRoomConfigWidget::MakePortals()
{
	// The current runtime owns destinations on placed APortal actors, not RoomData.
	// Do not recreate removed per-room fields or show controls which cannot be saved.
	return DevKitSceneRoomConfig::Note(LOCTEXT("PlacedPortalDestination",
		"请选中场景中的传送门，在 Details > Portal > Destination Level 设置目标关卡。"
		"目标房间配置和预骰 Buff 由现有 Open / TransitionToLevel 流程传递；此处不再维护旧的逐门候选池或强制 Index。"));
}

TSharedRef<SWidget> SDevKitSceneRoomConfigWidget::BuildGameplay()
{
	// Mobs and doors are owned by the map's AYogRoomDirector and its Lua module, not by RoomData.
	const TSharedRef<SVerticalBox> Cards = SNew(SVerticalBox);
	Cards->AddSlot().AutoHeight().Padding(0, 0, 0, 10)
		[MakeCard(LOCTEXT("RoomFlowTitle", "房间流程"),
			LOCTEXT("RoomFlowDescription", "敌人刷新与开门由地图中的 AYogRoomDirector 及其 Lua 模块负责。"),
			DevKitSceneRoomConfig::Note(LOCTEXT("RoomFlowNote",
				"在场景中放置 MobSpawner 和 RoomDirector，并在 Details 里指定 Lua Module；RoomData 不再保存刷怪、奖励或商店配置。")),
			DevKitSceneRoomConfig::GameplayAccent)];
	Cards->AddSlot().AutoHeight().Padding(0, 0, 0, 10)
		[MakeCard(LOCTEXT("PortalsTitle", "传送门与关卡去向"),
			LOCTEXT("PortalsDescription", "使用当前场景传送门流程；关卡去向不再保存在 RoomData 中。"),
			MakePortals(), DevKitSceneRoomConfig::DefaultsAccent)];
	return SNew(SScrollBox) + SScrollBox::Slot()[Cards];
}

#if WITH_DEV_AUTOMATION_TESTS
bool SDevKitSceneRoomConfigWidget::HasPropertyForTesting(FName Name) const
{
	return FindHandle(Name).IsValid();
}

bool SDevKitSceneRoomConfigWidget::AddArrayItemForTesting(FName Name)
{
	const TSharedPtr<IPropertyHandle> Handle = FindHandle(Name);
	const TSharedPtr<IPropertyHandleArray> Array = Handle.IsValid() ? Handle->AsArray() : nullptr;
	return Array.IsValid() && Array->AddItem().GetAccessResult() == FPropertyAccess::Success;
}

bool SDevKitSceneRoomConfigWidget::DeleteArrayItemForTesting(FName Name, int32 Index)
{
	const TSharedPtr<IPropertyHandle> Handle = FindHandle(Name);
	const TSharedPtr<IPropertyHandleArray> Array = Handle.IsValid() ? Handle->AsArray() : nullptr;
	return Array.IsValid() && Array->DeleteItem(Index) == FPropertyAccess::Success;
}

TSharedPtr<IPropertyHandle> SDevKitSceneRoomConfigWidget::GetArrayElementPropertyForTesting(
	FName ArrayName, int32 Index, FName ChildName) const
{
	const TSharedPtr<IPropertyHandle> Handle = FindHandle(ArrayName);
	const TSharedPtr<IPropertyHandleArray> Array = Handle.IsValid() ? Handle->AsArray() : nullptr;
	uint32 Count = 0;
	if (!Array.IsValid() || Array->GetNumElements(Count) != FPropertyAccess::Success
		|| Index < 0 || static_cast<uint32>(Index) >= Count)
	{
		return nullptr;
	}
	const TSharedRef<IPropertyHandle> Element = Array->GetElement(Index);
	if (ChildName.IsNone())
	{
		return Element;
	}
	return Element->GetChildHandle(ChildName);
}
#endif

#undef LOCTEXT_NAMESPACE
