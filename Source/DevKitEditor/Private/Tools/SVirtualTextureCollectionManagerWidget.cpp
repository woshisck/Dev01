#include "SVirtualTextureCollectionManagerWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCollection.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "PixelFormat.h"
#include "Styling/AppStyle.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"
#include "Tools/DevKitArtToolUI.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "DevKitTextureCollectionManager"

namespace
{
	const FString DefaultCollectionFolder(TEXT("/Game/Art/Textures/TextureCollection"));

	const FName ColCollectionName(TEXT("Name"));
	const FName ColCollectionMembers(TEXT("Members"));
	const FName ColCollectionMaxSize(TEXT("MaxSize"));
	const FName ColCollectionType(TEXT("Type"));
	const FName ColCollectionIssues(TEXT("Issues"));
	const FName ColCollectionPath(TEXT("Path"));

	const FName ColMemIndex(TEXT("Idx"));
	const FName ColMemName(TEXT("Texture"));
	const FName ColMemSize(TEXT("Size"));
	const FName ColMemFormat(TEXT("Format"));
	const FName ColMemVT(TEXT("VT"));
	const FName ColMemOK(TEXT("OK"));

	int32 GetMaxSide(UTexture* Tex)
	{
		if (UTexture2D* Tex2D = Cast<UTexture2D>(Tex))
		{
			return FMath::Max(static_cast<int32>(Tex2D->GetSizeX()), static_cast<int32>(Tex2D->GetSizeY()));
		}
		return 0;
	}

	bool IsPowerOfTwo(int32 V)
	{
		return V > 0 && (V & (V - 1)) == 0;
	}

	bool IsRecommendedPixelFormat(EPixelFormat Format)
	{
		switch (Format)
		{
		case PF_DXT1:
		case PF_DXT3:
		case PF_DXT5:
		case PF_BC4:
		case PF_BC5:
		case PF_BC6H:
		case PF_BC7:
		case PF_B8G8R8A8:
		case PF_R8G8B8A8:
		case PF_G8:
		case PF_R8G8:
			return true;
		default:
			return false;
		}
	}

	int64 EstimateVRAMBytes(UTexture2D* Tex2D)
	{
		if (!Tex2D)
		{
			return 0;
		}

		const EPixelFormat PF = Tex2D->GetPixelFormat();
		const int32 BlockBytes = GPixelFormats[PF].BlockBytes;
		const int32 BlockSizeX = FMath::Max(1, GPixelFormats[PF].BlockSizeX);
		const int32 BlockSizeY = FMath::Max(1, GPixelFormats[PF].BlockSizeY);
		return static_cast<int64>(FMath::DivideAndRoundUp<int32>(Tex2D->GetSizeX(), BlockSizeX)
			* FMath::DivideAndRoundUp<int32>(Tex2D->GetSizeY(), BlockSizeY) * BlockBytes) * 4 / 3;
	}
}

void SVirtualTextureCollectionManagerWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 12.f, 12.f, 8.f)
		[
			DevKitArtToolUI::MakeHeader(
				LOCTEXT("Title", "Texture Collection 管理器"),
				LOCTEXT("Description", "创建和维护普通 Texture Collection，检查成员是否保持 NoVT，并集中处理成员增删与兼容性问题。"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 0.f, 12.f, 6.f)
		[
			DevKitArtToolUI::MakeSectionHeader(1, LOCTEXT("ManageSection", "Collection 管理"), LOCTEXT("ManageSectionDesc", "左侧选择 Collection，右侧查看成员。新建操作与当前 Collection 操作已分开。"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 0.f, 12.f, 6.f)
		[
			BuildToolbar()
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(12.f, 0.f, 12.f, 6.f)
		[
			BuildSplitPanels()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(12.f, 0.f, 12.f, 12.f)
		[
			SNew(SBorder)
			.Padding(8.f)
			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			[
				SAssignNew(ActionStatusTextBlock, STextBlock)
				.Text(LOCTEXT("InitialActionStatus", "就绪。选择一个 Collection 后可追加、移除、校验或打开。"))
				.AutoWrapText(true)
			]
		]
	];

	ScanTextureCollections();
}

TSharedRef<SWidget> SVirtualTextureCollectionManagerWidget::BuildToolbar()
{
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("Refresh", "刷新列表"))
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::RefreshList)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("New", "新建 Texture Collection"))
		.ToolTipText(FText::Format(LOCTEXT("NewTip", "在 {0} 下创建空的 TextureCollection。"), FText::FromString(DefaultCollectionFolder)))
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::CreateNewTextureCollection)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("FromCB", "用选中贴图新建 Collection"))
		.ToolTipText(LOCTEXT("FromCBTip", "把 Content Browser 里当前选中的所有普通 Texture 打进一个新的 Texture Collection。源贴图不需要也不应该开启 VT。"))
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::CreateTextureCollectionFromContentBrowserSelection)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("AddToCurrent", "追加到当前 Collection"))
		.ToolTipText(LOCTEXT("AddToCurrentTip", "把 Content Browser 选中的 Texture 添加到左侧当前 Texture Collection。"))
		.IsEnabled(this, &SVirtualTextureCollectionManagerWidget::HasCurrentCollection)
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::AddContentBrowserSelectionToCurrentTextureCollection)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("Remove", "移除所选成员"))
		.IsEnabled(this, &SVirtualTextureCollectionManagerWidget::HasSelectedMembers)
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::RemoveSelectedMembersFromCurrentTextureCollection)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("Validate", "全部校验"))
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::ValidateAll)
	]
	+ SHorizontalBox::Slot().AutoWidth()
	[
		SNew(SButton)
		.Text(LOCTEXT("Open", "打开当前 Collection"))
		.IsEnabled(this, &SVirtualTextureCollectionManagerWidget::HasCurrentCollection)
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::OpenCurrentTextureCollection)
	]
	+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center)
	[
		SNew(STextBlock).Text(this, &SVirtualTextureCollectionManagerWidget::GetSummaryText)
	];
}

TSharedRef<SWidget> SVirtualTextureCollectionManagerWidget::BuildSplitPanels()
{
	return SNew(SSplitter)
	.Orientation(Orient_Horizontal)
	+ SSplitter::Slot().Value(0.42f)
	[
		BuildCollectionListPanel()
	]
	+ SSplitter::Slot().Value(0.58f)
	[
		BuildDetailPanel()
	];
}

TSharedRef<SWidget> SVirtualTextureCollectionManagerWidget::BuildCollectionListPanel()
{
	return SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
	[
		SAssignNew(CollectionListView, SListView<FCollectionEntryPtr>)
		.ListItemsSource(&AllEntries)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SVirtualTextureCollectionManagerWidget::GenerateCollectionRow)
		.OnSelectionChanged(this, &SVirtualTextureCollectionManagerWidget::OnCollectionSelectionChanged)
		.HeaderRow(
			SNew(SHeaderRow)
			+ SHeaderRow::Column(ColCollectionName).DefaultLabel(LOCTEXT("HName", "名称")).FillWidth(0.35f)
			+ SHeaderRow::Column(ColCollectionMembers).DefaultLabel(LOCTEXT("HMembers", "数量")).FixedWidth(60)
			+ SHeaderRow::Column(ColCollectionMaxSize).DefaultLabel(LOCTEXT("HMaxSize", "最大尺寸")).FixedWidth(80)
			+ SHeaderRow::Column(ColCollectionType).DefaultLabel(LOCTEXT("HType", "类型")).FixedWidth(130)
			+ SHeaderRow::Column(ColCollectionIssues).DefaultLabel(LOCTEXT("HIssues", "问题")).FixedWidth(60)
			+ SHeaderRow::Column(ColCollectionPath).DefaultLabel(LOCTEXT("HPath", "路径")).FillWidth(0.5f)
		)
	];
}

TSharedRef<SWidget> SVirtualTextureCollectionManagerWidget::BuildDetailPanel()
{
	return SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6, 6)
		[
			SNew(STextBlock)
			.Text(this, &SVirtualTextureCollectionManagerWidget::GetDetailHeaderText)
			.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
		]
		+ SVerticalBox::Slot().FillHeight(0.65f).Padding(6, 0, 6, 6)
		[
			SAssignNew(MemberListView, SListView<TSharedPtr<int32>>)
			.ListItemsSource(&CurrentMemberIndices)
			.SelectionMode(ESelectionMode::Multi)
			.OnGenerateRow(this, &SVirtualTextureCollectionManagerWidget::GenerateMemberRow)
			.OnSelectionChanged(this, &SVirtualTextureCollectionManagerWidget::OnMemberSelectionChanged)
			.HeaderRow(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(ColMemIndex).DefaultLabel(LOCTEXT("HIdx", "索引")).FixedWidth(45)
				+ SHeaderRow::Column(ColMemName).DefaultLabel(LOCTEXT("HTex", "贴图")).FillWidth(0.4f)
				+ SHeaderRow::Column(ColMemSize).DefaultLabel(LOCTEXT("HSize", "尺寸")).FixedWidth(80)
				+ SHeaderRow::Column(ColMemFormat).DefaultLabel(LOCTEXT("HFmt", "格式")).FixedWidth(120)
				+ SHeaderRow::Column(ColMemVT).DefaultLabel(LOCTEXT("HVT", "源VT")).FixedWidth(50)
				+ SHeaderRow::Column(ColMemOK).DefaultLabel(LOCTEXT("HOK", "OK")).FillWidth(0.3f)
			)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(6, 4)[SNew(SSeparator)]
		+ SVerticalBox::Slot().FillHeight(0.35f).Padding(6)
		[
			SNew(STextBlock)
			.Text(this, &SVirtualTextureCollectionManagerWidget::GetDetailIssuesText)
			.AutoWrapText(true)
		]
	];
}

class STextureCollectionRow : public SMultiColumnTableRow<TSharedPtr<FTextureCollectionEntry>>
{
public:
	SLATE_BEGIN_ARGS(STextureCollectionRow) {}
		SLATE_ARGUMENT(TSharedPtr<FTextureCollectionEntry>, Entry)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
	{
		Entry = InArgs._Entry;
		SMultiColumnTableRow::Construct(FSuperRowType::FArguments(), OwnerTable);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Col) override
	{
		if (Col == ColCollectionName)    return SNew(STextBlock).Text(FText::FromString(Entry->AssetName));
		if (Col == ColCollectionMembers) return SNew(STextBlock).Text(FText::AsNumber(Entry->MemberCount));
		if (Col == ColCollectionMaxSize) return SNew(STextBlock).Text(FText::AsNumber(Entry->MaxSize));
		if (Col == ColCollectionType)    return SNew(STextBlock).Text(FText::FromString(Entry->CollectionType));
		if (Col == ColCollectionIssues)
		{
			const int32 IssueCount = Entry->Issues.Num();
			return SNew(STextBlock)
				.Text(FText::AsNumber(IssueCount))
				.ColorAndOpacity(IssueCount > 0 ? FLinearColor(1.f, 0.4f, 0.4f) : FLinearColor(0.4f, 0.9f, 0.4f));
		}
		if (Col == ColCollectionPath) return SNew(STextBlock).Text(FText::FromString(Entry->PackagePath));
		return SNullWidget::NullWidget;
	}

	TSharedPtr<FTextureCollectionEntry> Entry;
};

TSharedRef<ITableRow> SVirtualTextureCollectionManagerWidget::GenerateCollectionRow(FCollectionEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STextureCollectionRow, OwnerTable).Entry(Entry);
}

class STextureCollectionMemberRow : public SMultiColumnTableRow<TSharedPtr<int32>>
{
public:
	SLATE_BEGIN_ARGS(STextureCollectionMemberRow) {}
		SLATE_ARGUMENT(TSharedPtr<int32>, IndexPtr)
		SLATE_ARGUMENT(TSharedPtr<FTextureCollectionEntry>, Entry)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
	{
		IndexPtr = InArgs._IndexPtr;
		Entry = InArgs._Entry;
		SMultiColumnTableRow::Construct(FSuperRowType::FArguments(), OwnerTable);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Col) override
	{
		if (!IndexPtr.IsValid() || !Entry.IsValid() || !Entry->Collection.IsValid()) return SNullWidget::NullWidget;
		const int32 Idx = *IndexPtr;
		UTextureCollection* Collection = Entry->Collection.Get();
		if (Idx < 0 || Idx >= Collection->Textures.Num()) return SNullWidget::NullWidget;
		UTexture* Tex = Collection->Textures[Idx];

		if (Col == ColMemIndex) return SNew(STextBlock).Text(FText::AsNumber(Idx));
		if (Col == ColMemName)
		{
			return SNew(STextBlock).Text(FText::FromString(Tex ? Tex->GetName() : TEXT("<null>")))
				.ColorAndOpacity(Tex ? FLinearColor::White : FLinearColor(1.f, 0.3f, 0.3f));
		}
		if (UTexture2D* Tex2D = Cast<UTexture2D>(Tex))
		{
			if (Col == ColMemSize)   return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%dx%d"), Tex2D->GetSizeX(), Tex2D->GetSizeY())));
			if (Col == ColMemFormat) return SNew(STextBlock).Text(FText::FromString(GPixelFormats[Tex2D->GetPixelFormat()].Name));
			if (Col == ColMemVT)
			{
				return SNew(STextBlock)
					.Text(Tex2D->VirtualTextureStreaming ? LOCTEXT("MemberVTYes", "Yes") : LOCTEXT("MemberVTNo", "No"))
					.ColorAndOpacity(Tex2D->VirtualTextureStreaming ? FLinearColor(1.f, 0.25f, 0.25f) : FLinearColor(0.4f, 0.9f, 0.4f));
			}
			if (Col == ColMemOK)
			{
				FText Reason;
				const bool bOK = SVirtualTextureCollectionManagerWidget::IsTextureCompatibleForCollection(Tex2D, Reason);
				return SNew(STextBlock).Text(bOK ? LOCTEXT("MemOK", "OK") : Reason)
					.ColorAndOpacity(bOK ? FLinearColor(0.4f, 0.9f, 0.4f) : FLinearColor(1.f, 0.4f, 0.4f))
					.AutoWrapText(true);
			}
		}
		return SNew(STextBlock).Text(FText::FromString(TEXT("--")));
	}

	TSharedPtr<int32> IndexPtr;
	TSharedPtr<FTextureCollectionEntry> Entry;
};

TSharedRef<ITableRow> SVirtualTextureCollectionManagerWidget::GenerateMemberRow(TSharedPtr<int32> Index, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STextureCollectionMemberRow, OwnerTable).IndexPtr(Index).Entry(CurrentEntry);
}

void SVirtualTextureCollectionManagerWidget::OnCollectionSelectionChanged(FCollectionEntryPtr Entry, ESelectInfo::Type)
{
	CurrentEntry = Entry;
	CurrentMemberIndices.Reset();
	if (Entry.IsValid() && Entry->Collection.IsValid())
	{
		const int32 Num = Entry->Collection->Textures.Num();
		for (int32 i = 0; i < Num; ++i)
		{
			CurrentMemberIndices.Add(MakeShared<int32>(i));
		}
	}
	SelectedMemberIndices.Reset();
	if (MemberListView.IsValid())
	{
		MemberListView->RequestListRefresh();
	}
}

void SVirtualTextureCollectionManagerWidget::OnMemberSelectionChanged(TSharedPtr<int32>, ESelectInfo::Type)
{
	SelectedMemberIndices.Reset();
	if (MemberListView.IsValid())
	{
		SelectedMemberIndices = MemberListView->GetSelectedItems();
	}
}

FReply SVirtualTextureCollectionManagerWidget::RefreshList()
{
	ScanTextureCollections();
	SetActionStatus(LOCTEXT("RefreshedStatus", "已刷新 Texture Collection 列表。"));
	return FReply::Handled();
}

void SVirtualTextureCollectionManagerWidget::ScanTextureCollections()
{
	AllEntries.Reset();
	CurrentEntry.Reset();
	CurrentMemberIndices.Reset();

	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AR = ARM.Get();
	if (AR.IsLoadingAssets())
	{
		AR.SearchAllAssets(true);
	}

	FARFilter Filter;
	Filter.ClassPaths.Add(UTextureCollection::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = false;

	TArray<FAssetData> Assets;
	AR.GetAssets(Filter, Assets);

	for (const FAssetData& AD : Assets)
	{
		FCollectionEntryPtr E = MakeShared<FTextureCollectionEntry>();
		E->AssetData = AD;
		E->AssetName = AD.AssetName.ToString();
		E->PackagePath = AD.PackagePath.ToString();

		if (UTextureCollection* Collection = Cast<UTextureCollection>(AD.GetAsset()))
		{
			if (Collection->IsVirtualCollection())
			{
				continue;
			}

			E->Collection = Collection;
			E->MemberCount = Collection->Textures.Num();
			E->CollectionType = TEXT("TextureCollection");

			int32 MaxSide = 0;
			int64 VRAM = 0;
			for (UTexture* Tex : Collection->Textures)
			{
				MaxSide = FMath::Max(MaxSide, GetMaxSide(Tex));
				VRAM += EstimateVRAMBytes(Cast<UTexture2D>(Tex));
			}
			E->MaxSize = MaxSide;
			E->TotalVRAMBytes = VRAM;
		}
		ValidateEntry(*E);
		AllEntries.Add(E);
	}

	AllEntries.Sort([](const FCollectionEntryPtr& A, const FCollectionEntryPtr& B) { return A->AssetName < B->AssetName; });

	if (CollectionListView.IsValid()) CollectionListView->RequestListRefresh();
	if (MemberListView.IsValid()) MemberListView->RequestListRefresh();
}

void SVirtualTextureCollectionManagerWidget::ValidateEntry(FTextureCollectionEntry& Entry) const
{
	Entry.Issues.Reset();
	if (!Entry.Collection.IsValid()) return;
	UTextureCollection* Collection = Entry.Collection.Get();

	for (int32 i = 0; i < Collection->Textures.Num(); ++i)
	{
		UTexture* Texture = Collection->Textures[i];
		if (!Texture)
		{
			FTextureCollectionMemberIssue Issue;
			Issue.TextureName = FString::Printf(TEXT("[%d]"), i);
			Issue.Message = LOCTEXT("NullEntry", "空成员，需要清理。");
			Issue.bBlocker = true;
			Entry.Issues.Add(Issue);
			continue;
		}

		FText Reason;
		if (!IsTextureCompatibleForCollection(Texture, Reason))
		{
			FTextureCollectionMemberIssue Issue;
			Issue.TextureName = Texture->GetName();
			Issue.Message = Reason;
			Issue.bBlocker = true;
			Entry.Issues.Add(Issue);
			continue;
		}

		if (UTexture2D* Tex2D = Cast<UTexture2D>(Texture))
		{
			const int32 MaxSide = GetMaxSide(Tex2D);
			if (MaxSide > 4096)
			{
				FTextureCollectionMemberIssue Issue;
				Issue.TextureName = Texture->GetName();
				Issue.Message = FText::Format(LOCTEXT("LargeTexture", "尺寸 {0} 超过 4K；Texture Collection 可容纳，但请确认显存和 mip 预算。"), FText::AsNumber(MaxSide));
				Issue.bBlocker = false;
				Entry.Issues.Add(Issue);
			}

			if (!IsPowerOfTwo(Tex2D->GetSizeX()) || !IsPowerOfTwo(Tex2D->GetSizeY()))
			{
				FTextureCollectionMemberIssue Issue;
				Issue.TextureName = Texture->GetName();
				Issue.Message = LOCTEXT("NPOTTexture", "非 2 的幂尺寸；可使用，但建议确认压缩和 mip。");
				Issue.bBlocker = false;
				Entry.Issues.Add(Issue);
			}

			if (!IsRecommendedPixelFormat(Tex2D->GetPixelFormat()))
			{
				FTextureCollectionMemberIssue Issue;
				Issue.TextureName = Texture->GetName();
				Issue.Message = FText::Format(LOCTEXT("NonRecommendedFormat", "格式 {0} 不在项目推荐白名单中。"), FText::FromString(GPixelFormats[Tex2D->GetPixelFormat()].Name));
				Issue.bBlocker = false;
				Entry.Issues.Add(Issue);
			}
		}
	}
}

bool SVirtualTextureCollectionManagerWidget::IsTextureCompatibleForCollection(UTexture* Texture, FText& OutReason)
{
	UTexture2D* Tex2D = Cast<UTexture2D>(Texture);
	if (!Tex2D)
	{
		OutReason = LOCTEXT("NotTex2D", "只支持 Texture2D。");
		return false;
	}

	if (Tex2D->VirtualTextureStreaming)
	{
		OutReason = LOCTEXT("TextureVTOn", "源贴图开启了 VirtualTextureStreaming；普通 Texture Collection 路径要求 NoVT。");
		return false;
	}

	OutReason = FText::GetEmpty();
	return true;
}

TArray<UTexture*> SVirtualTextureCollectionManagerWidget::CollectTexturesFromContentBrowser()
{
	TArray<UTexture*> Out;
	FContentBrowserModule& CBM = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FAssetData> Selection;
	CBM.Get().GetSelectedAssets(Selection);
	for (const FAssetData& AD : Selection)
	{
		if (UTexture* T = Cast<UTexture>(AD.GetAsset()))
		{
			Out.Add(T);
		}
	}
	return Out;
}

UTextureCollection* SVirtualTextureCollectionManagerWidget::CreateTextureCollectionAsset(const FString& PackagePath, const FString& AssetName)
{
	FAssetToolsModule& ATM = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	IAssetTools& AssetTools = ATM.Get();
	FString UniqueName;
	FString UniquePkg;
	AssetTools.CreateUniqueAssetName(PackagePath / AssetName, TEXT(""), UniquePkg, UniqueName);
	UObject* NewObj = AssetTools.CreateAsset(UniqueName, PackagePath, UTextureCollection::StaticClass(), nullptr);
	return Cast<UTextureCollection>(NewObj);
}

void SVirtualTextureCollectionManagerWidget::AddTexturesToCollection(UTextureCollection* Collection, const TArray<UTexture*>& Textures)
{
	if (!Collection) return;
	Collection->Modify();
	for (UTexture* T : Textures)
	{
		FText Reason;
		if (T && !Collection->Textures.Contains(T) && IsTextureCompatibleForCollection(T, Reason))
		{
			Collection->Textures.Add(T);
		}
	}
	Collection->PostEditChange();
	Collection->MarkPackageDirty();
}

FReply SVirtualTextureCollectionManagerWidget::CreateNewTextureCollection()
{
	IFileManager::Get().MakeDirectory(*FPackageName::LongPackageNameToFilename(DefaultCollectionFolder), true);
	UTextureCollection* Collection = CreateTextureCollectionAsset(DefaultCollectionFolder, TEXT("TC_New"));
	if (Collection)
	{
		FEditorFileUtils::PromptForCheckoutAndSave({Collection->GetOutermost()}, false, false);
		ScanTextureCollections();
		SetActionStatus(FText::Format(LOCTEXT("CreatedCollectionStatus", "已创建并保存 {0}。"), FText::FromString(Collection->GetName())));
	}
	return FReply::Handled();
}

FReply SVirtualTextureCollectionManagerWidget::CreateTextureCollectionFromContentBrowserSelection()
{
	TArray<UTexture*> Selection = CollectTexturesFromContentBrowser();
	if (Selection.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoSel", "Content Browser 里没有选中任何 Texture。请先选中要加入 Texture Collection 的贴图。"));
		return FReply::Handled();
	}
	int32 CompatibleCount = 0;
	for (UTexture* Texture : Selection)
	{
		FText Reason;
		CompatibleCount += IsTextureCompatibleForCollection(Texture, Reason) ? 1 : 0;
	}
	if (CompatibleCount == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoCompatibleSelection", "当前选择中没有可加入的贴图。请先关闭源贴图 VT，并确认资产是 Texture2D。"));
		SetActionStatus(LOCTEXT("NoCompatibleSelectionStatus", "未创建 Collection：当前选择中没有兼容贴图。"));
		return FReply::Handled();
	}

	IFileManager::Get().MakeDirectory(*FPackageName::LongPackageNameToFilename(DefaultCollectionFolder), true);
	UTextureCollection* Collection = CreateTextureCollectionAsset(DefaultCollectionFolder, TEXT("TC_FromSelection"));
	if (Collection)
	{
		AddTexturesToCollection(Collection, Selection);
		const int32 AddedCount = Collection->Textures.Num();
		FEditorFileUtils::PromptForCheckoutAndSave({Collection->GetOutermost()}, false, false);
		ScanTextureCollections();
		SetActionStatus(FText::Format(LOCTEXT("CreatedFromSelectionStatus", "已创建 {0}，加入 {1} 张兼容贴图；不兼容项已跳过。"),
			FText::FromString(Collection->GetName()), FText::AsNumber(AddedCount)));
	}
	return FReply::Handled();
}

FReply SVirtualTextureCollectionManagerWidget::AddContentBrowserSelectionToCurrentTextureCollection()
{
	if (!CurrentEntry.IsValid() || !CurrentEntry->Collection.IsValid())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoCurrent", "请先在左侧列表选中一个 Texture Collection。"));
		return FReply::Handled();
	}

	TArray<UTexture*> Selection = CollectTexturesFromContentBrowser();
	if (Selection.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoSel2", "Content Browser 里没有选中任何 Texture。"));
		return FReply::Handled();
	}

	UTextureCollection* Collection = CurrentEntry->Collection.Get();
	const int32 BeforeCount = Collection->Textures.Num();
	AddTexturesToCollection(Collection, Selection);
	const int32 AddedCount = Collection->Textures.Num() - BeforeCount;
	FEditorFileUtils::PromptForCheckoutAndSave({CurrentEntry->Collection->GetOutermost()}, false, false);
	ScanTextureCollections();
	SetActionStatus(FText::Format(LOCTEXT("AddedMembersStatus", "已加入 {0} 张贴图；重复项或不兼容项已跳过。"), FText::AsNumber(AddedCount)));
	return FReply::Handled();
}

FReply SVirtualTextureCollectionManagerWidget::RemoveSelectedMembersFromCurrentTextureCollection()
{
	if (!CurrentEntry.IsValid() || !CurrentEntry->Collection.IsValid() || SelectedMemberIndices.IsEmpty())
	{
		return FReply::Handled();
	}
	UTextureCollection* Collection = CurrentEntry->Collection.Get();
	const EAppReturnType::Type Confirmation = FMessageDialog::Open(
		EAppMsgType::YesNo,
		FText::Format(LOCTEXT("ConfirmRemoveMembers", "确定从 {0} 中移除所选 {1} 个成员吗？\n\n操作支持撤销。"),
			FText::FromString(Collection->GetName()), FText::AsNumber(SelectedMemberIndices.Num())));
	if (Confirmation != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("RemoveMembersTransaction", "移除 Texture Collection 成员"));

	TArray<int32> Indices;
	for (const TSharedPtr<int32>& P : SelectedMemberIndices)
	{
		if (P.IsValid())
		{
			Indices.Add(*P);
		}
	}
	Indices.Sort([](int32 A, int32 B) { return A > B; });

	Collection->Modify();
	for (int32 I : Indices)
	{
		if (Collection->Textures.IsValidIndex(I))
		{
			Collection->Textures.RemoveAt(I);
		}
	}
	Collection->PostEditChange();
	Collection->MarkPackageDirty();
	FEditorFileUtils::PromptForCheckoutAndSave({Collection->GetOutermost()}, false, false);
	SetActionStatus(FText::Format(LOCTEXT("RemovedMembersStatus", "已移除 {0} 个成员并保存 Collection，可使用 Ctrl+Z 撤销。"), FText::AsNumber(Indices.Num())));
	ScanTextureCollections();
	return FReply::Handled();
}

FReply SVirtualTextureCollectionManagerWidget::OpenCurrentTextureCollection()
{
	if (CurrentEntry.IsValid() && CurrentEntry->Collection.IsValid() && GEditor)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(CurrentEntry->Collection.Get());
	}
	return FReply::Handled();
}

FReply SVirtualTextureCollectionManagerWidget::ValidateAll()
{
	for (const FCollectionEntryPtr& E : AllEntries)
	{
		ValidateEntry(*E);
	}
	if (CollectionListView.IsValid())
	{
		CollectionListView->RequestListRefresh();
	}
	SetActionStatus(LOCTEXT("ValidatedStatus", "已重新校验全部 Texture Collection；请优先处理标记为阻塞的问题。"));
	return FReply::Handled();
}

FText SVirtualTextureCollectionManagerWidget::GetSummaryText() const
{
	int32 IssueTotal = 0;
	int32 BlockerTotal = 0;
	int32 MemberTotal = 0;
	int64 VRAM = 0;
	for (const FCollectionEntryPtr& E : AllEntries)
	{
		IssueTotal += E->Issues.Num();
		for (const FTextureCollectionMemberIssue& Issue : E->Issues)
		{
			BlockerTotal += Issue.bBlocker ? 1 : 0;
		}
		MemberTotal += E->MemberCount;
		VRAM += E->TotalVRAMBytes;
	}
	return FText::Format(
		LOCTEXT("CollectionSummary", "Texture Collection {0} | 成员 {1} | 问题 {2} | 阻塞 {3} | 估算 VRAM {4} MB"),
		FText::AsNumber(AllEntries.Num()),
		FText::AsNumber(MemberTotal),
		FText::AsNumber(IssueTotal),
		FText::AsNumber(BlockerTotal),
		FText::AsNumber(VRAM / 1048576));
}

FText SVirtualTextureCollectionManagerWidget::GetDetailHeaderText() const
{
	if (!CurrentEntry.IsValid())
	{
		return LOCTEXT("NoEntry", "在左侧选择一个 Texture Collection 查看成员。");
	}

	return FText::Format(LOCTEXT("Detail", "{0} - {1} 个成员，最大尺寸 {2}，类型 {3}"),
		FText::FromString(CurrentEntry->AssetName),
		FText::AsNumber(CurrentEntry->MemberCount),
		FText::AsNumber(CurrentEntry->MaxSize),
		FText::FromString(CurrentEntry->CollectionType));
}

FText SVirtualTextureCollectionManagerWidget::GetDetailIssuesText() const
{
	if (!CurrentEntry.IsValid())
	{
		return FText::GetEmpty();
	}
	if (CurrentEntry->Issues.IsEmpty())
	{
		return LOCTEXT("NoIssues", "无问题。所有成员通过 Texture Collection / NoVT 检查。");
	}

	FString Text;
	for (const FTextureCollectionMemberIssue& Issue : CurrentEntry->Issues)
	{
		Text += FString::Printf(TEXT("- %s: %s%s\n"),
			*Issue.TextureName,
			*Issue.Message.ToString(),
			Issue.bBlocker ? TEXT(" [阻塞]") : TEXT(""));
	}
	return FText::FromString(Text);
}

bool SVirtualTextureCollectionManagerWidget::HasCurrentCollection() const
{
	return CurrentEntry.IsValid() && CurrentEntry->Collection.IsValid();
}

bool SVirtualTextureCollectionManagerWidget::HasSelectedMembers() const
{
	return HasCurrentCollection() && !SelectedMemberIndices.IsEmpty();
}

void SVirtualTextureCollectionManagerWidget::SetActionStatus(const FText& InText)
{
	if (ActionStatusTextBlock.IsValid())
	{
		ActionStatusTextBlock->SetText(InText);
	}
}

#undef LOCTEXT_NAMESPACE
