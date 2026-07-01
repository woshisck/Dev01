#include "SVirtualTextureCollectionManagerWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCollection.h"
#include "Engine/VirtualTextureCollection.h"
#include "FileHelpers.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "PixelFormat.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/SavePackage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitVTCManager"

namespace
{
	const FName ColVTCName(TEXT("Name"));
	const FName ColVTCMembers(TEXT("Members"));
	const FName ColVTCMaxSize(TEXT("MaxSize"));
	const FName ColVTCFormat(TEXT("Format"));
	const FName ColVTCIssues(TEXT("Issues"));
	const FName ColVTCPath(TEXT("Path"));

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
}

void SVirtualTextureCollectionManagerWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4)
		[
			BuildToolbar()
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(4)
		[
			BuildSplitPanels()
		]
	];

	ScanVTCs();
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
		.Text(LOCTEXT("New", "新建 VTC"))
		.ToolTipText(LOCTEXT("NewTip", "在 /Game/Art/Textures/VTC 下创建空的 VirtualTextureCollection。"))
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::CreateNewVTC)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("FromCB", "用 Content Browser 选中贴图新建 VTC"))
		.ToolTipText(LOCTEXT("FromCBTip", "把 Content Browser 里当前选中的所有 Texture 打包进一个新 VTC。"))
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::CreateVTCFromContentBrowserSelection)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("AddToCurrent", "追加到当前 VTC"))
		.ToolTipText(LOCTEXT("AddToCurrentTip", "把 Content Browser 选中的 Texture 添加到左侧列表选中的 VTC。"))
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::AddContentBrowserSelectionToCurrentVTC)
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
	[
		SNew(SButton)
		.Text(LOCTEXT("Remove", "移除所选成员"))
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::RemoveSelectedMembersFromCurrentVTC)
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
		.Text(LOCTEXT("Open", "打开当前 VTC"))
		.OnClicked(this, &SVirtualTextureCollectionManagerWidget::OpenCurrentVTC)
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
		BuildVTCListPanel()
	]
	+ SSplitter::Slot().Value(0.58f)
	[
		BuildDetailPanel()
	];
}

TSharedRef<SWidget> SVirtualTextureCollectionManagerWidget::BuildVTCListPanel()
{
	return SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
	[
		SAssignNew(VTCListView, SListView<FVTCEntryPtr>)
		.ListItemsSource(&AllEntries)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SVirtualTextureCollectionManagerWidget::GenerateVTCRow)
		.OnSelectionChanged(this, &SVirtualTextureCollectionManagerWidget::OnVTCSelectionChanged)
		.HeaderRow(
			SNew(SHeaderRow)
			+ SHeaderRow::Column(ColVTCName).DefaultLabel(LOCTEXT("HName", "名称")).FillWidth(0.35f)
			+ SHeaderRow::Column(ColVTCMembers).DefaultLabel(LOCTEXT("HMembers", "数量")).FixedWidth(60)
			+ SHeaderRow::Column(ColVTCMaxSize).DefaultLabel(LOCTEXT("HMaxSize", "最大尺寸")).FixedWidth(80)
			+ SHeaderRow::Column(ColVTCFormat).DefaultLabel(LOCTEXT("HFormat", "运行时格式")).FixedWidth(120)
			+ SHeaderRow::Column(ColVTCIssues).DefaultLabel(LOCTEXT("HIssues", "问题")).FixedWidth(60)
			+ SHeaderRow::Column(ColVTCPath).DefaultLabel(LOCTEXT("HPath", "路径")).FillWidth(0.5f)
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
				+ SHeaderRow::Column(ColMemVT).DefaultLabel(LOCTEXT("HVT", "VT")).FixedWidth(40)
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

/* ============ VTC 行 ============ */
class SVTCRow : public SMultiColumnTableRow<TSharedPtr<FVTCEntry>>
{
public:
	SLATE_BEGIN_ARGS(SVTCRow) {}
		SLATE_ARGUMENT(TSharedPtr<FVTCEntry>, Entry)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
	{
		Entry = InArgs._Entry;
		SMultiColumnTableRow::Construct(FSuperRowType::FArguments(), OwnerTable);
	}
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Col) override
	{
		if (Col == ColVTCName)    return SNew(STextBlock).Text(FText::FromString(Entry->AssetName));
		if (Col == ColVTCMembers) return SNew(STextBlock).Text(FText::AsNumber(Entry->MemberCount));
		if (Col == ColVTCMaxSize) return SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%d"), Entry->MaxSize)));
		if (Col == ColVTCFormat)  return SNew(STextBlock).Text(FText::FromString(Entry->RuntimeFormat));
		if (Col == ColVTCIssues)
		{
			const int32 IssueCount = Entry->Issues.Num();
			return SNew(STextBlock)
				.Text(FText::AsNumber(IssueCount))
				.ColorAndOpacity(IssueCount > 0 ? FLinearColor(1.f, 0.4f, 0.4f) : FLinearColor(0.4f, 0.9f, 0.4f));
		}
		if (Col == ColVTCPath)    return SNew(STextBlock).Text(FText::FromString(Entry->PackagePath));
		return SNullWidget::NullWidget;
	}
	TSharedPtr<FVTCEntry> Entry;
};

TSharedRef<ITableRow> SVirtualTextureCollectionManagerWidget::GenerateVTCRow(FVTCEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SVTCRow, OwnerTable).Entry(Entry);
}

/* ============ Member 行 ============ */
class SVTCMemberRow : public SMultiColumnTableRow<TSharedPtr<int32>>
{
public:
	SLATE_BEGIN_ARGS(SVTCMemberRow) {}
		SLATE_ARGUMENT(TSharedPtr<int32>, IndexPtr)
		SLATE_ARGUMENT(TSharedPtr<FVTCEntry>, Entry)
	SLATE_END_ARGS()
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable)
	{
		IndexPtr = InArgs._IndexPtr;
		Entry = InArgs._Entry;
		SMultiColumnTableRow::Construct(FSuperRowType::FArguments(), OwnerTable);
	}
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& Col) override
	{
		if (!IndexPtr.IsValid() || !Entry.IsValid() || !Entry->VTC.IsValid()) return SNullWidget::NullWidget;
		const int32 Idx = *IndexPtr;
		UVirtualTextureCollection* VTC = Entry->VTC.Get();
		if (Idx < 0 || Idx >= VTC->Textures.Num()) return SNullWidget::NullWidget;
		UTexture* Tex = VTC->Textures[Idx];

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
			if (Col == ColMemVT)     return SNew(STextBlock).Text(FText::FromString(Tex2D->VirtualTextureStreaming ? TEXT("Yes") : TEXT("No")))
				.ColorAndOpacity(Tex2D->VirtualTextureStreaming ? FLinearColor(0.4f, 0.9f, 0.4f) : FLinearColor(1.f, 0.4f, 0.4f));
			if (Col == ColMemOK)
			{
				FText Reason;
				const bool bOK = SVirtualTextureCollectionManagerWidget::IsTextureCompatibleForVTC(Tex2D, Reason);
				return SNew(STextBlock).Text(bOK ? LOCTEXT("MemOK", "OK") : Reason)
					.ColorAndOpacity(bOK ? FLinearColor(0.4f, 0.9f, 0.4f) : FLinearColor(1.f, 0.4f, 0.4f))
					.AutoWrapText(true);
			}
		}
		return SNew(STextBlock).Text(FText::FromString(TEXT("--")));
	}
	TSharedPtr<int32> IndexPtr;
	TSharedPtr<FVTCEntry> Entry;
};

TSharedRef<ITableRow> SVirtualTextureCollectionManagerWidget::GenerateMemberRow(TSharedPtr<int32> Index, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SVTCMemberRow, OwnerTable).IndexPtr(Index).Entry(CurrentEntry);
}

void SVirtualTextureCollectionManagerWidget::OnVTCSelectionChanged(FVTCEntryPtr Entry, ESelectInfo::Type)
{
	CurrentEntry = Entry;
	CurrentMemberIndices.Reset();
	if (Entry.IsValid() && Entry->VTC.IsValid())
	{
		const int32 Num = Entry->VTC->Textures.Num();
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
	ScanVTCs();
	return FReply::Handled();
}

void SVirtualTextureCollectionManagerWidget::ScanVTCs()
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
	Filter.ClassPaths.Add(UVirtualTextureCollection::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(TEXT("/Game"));
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AR.GetAssets(Filter, Assets);

	for (const FAssetData& AD : Assets)
	{
		FVTCEntryPtr E = MakeShared<FVTCEntry>();
		E->AssetData = AD;
		E->AssetName = AD.AssetName.ToString();
		E->PackagePath = AD.PackagePath.ToString();

		if (UVirtualTextureCollection* VTC = Cast<UVirtualTextureCollection>(AD.GetAsset()))
		{
			E->VTC = VTC;
			E->MemberCount = VTC->Textures.Num();
			E->bSRGB = VTC->bIsSRGB;
			E->RuntimeFormat = FString(GPixelFormats[VTC->RuntimePixelFormat].Name);

			int32 MaxSide = 0;
			int64 VRAM = 0;
			for (UTexture* Tex : VTC->Textures)
			{
				const int32 S = GetMaxSide(Tex);
				MaxSide = FMath::Max(MaxSide, S);
				if (UTexture2D* Tex2D = Cast<UTexture2D>(Tex))
				{
					const EPixelFormat PF = Tex2D->GetPixelFormat();
					const int32 BB = GPixelFormats[PF].BlockBytes;
					const int32 BX = FMath::Max(1, GPixelFormats[PF].BlockSizeX);
					const int32 BY = FMath::Max(1, GPixelFormats[PF].BlockSizeY);
					VRAM += static_cast<int64>(FMath::DivideAndRoundUp<int32>(Tex2D->GetSizeX(), BX)
						* FMath::DivideAndRoundUp<int32>(Tex2D->GetSizeY(), BY) * BB) * 4 / 3;
				}
			}
			E->MaxSize = MaxSide;
			E->TotalVRAMBytes = VRAM;
		}
		ValidateEntry(*E);
		AllEntries.Add(E);
	}

	AllEntries.Sort([](const FVTCEntryPtr& A, const FVTCEntryPtr& B) { return A->AssetName < B->AssetName; });

	if (VTCListView.IsValid())    VTCListView->RequestListRefresh();
	if (MemberListView.IsValid()) MemberListView->RequestListRefresh();
}

void SVirtualTextureCollectionManagerWidget::ValidateEntry(FVTCEntry& Entry) const
{
	Entry.Issues.Reset();
	if (!Entry.VTC.IsValid()) return;
	UVirtualTextureCollection* VTC = Entry.VTC.Get();

	for (int32 i = 0; i < VTC->Textures.Num(); ++i)
	{
		UTexture* T = VTC->Textures[i];
		if (!T)
		{
			FVTCMemberIssue Iss;
			Iss.TextureName = FString::Printf(TEXT("[%d]"), i);
			Iss.Message = LOCTEXT("NullEntry", "空成员，需要清理。");
			Iss.bBlocker = true;
			Entry.Issues.Add(Iss);
			continue;
		}
		FText Reason;
		if (!IsTextureCompatibleForVTC(T, Reason))
		{
			FVTCMemberIssue Iss;
			Iss.TextureName = T->GetName();
			Iss.Message = Reason;
			Iss.bBlocker = true;
			Entry.Issues.Add(Iss);
		}
	}
}

bool SVirtualTextureCollectionManagerWidget::IsTextureCompatibleForVTC(UTexture* Texture, FText& OutReason)
{
	UTexture2D* Tex2D = Cast<UTexture2D>(Texture);
	if (!Tex2D)
	{
		OutReason = LOCTEXT("NotTex2D", "只支持 Texture2D。");
		return false;
	}
	const int32 Side = FMath::Max<int32>(Tex2D->GetSizeX(), Tex2D->GetSizeY());
	if (Side > 4096)
	{
		OutReason = FText::Format(LOCTEXT("TooBig", "尺寸 {0} > 4K 上限。"), FText::AsNumber(Side));
		return false;
	}
	const EPixelFormat PF = Tex2D->GetPixelFormat();
	switch (PF)
	{
	case PF_DXT1: case PF_DXT3: case PF_DXT5:
	case PF_BC4: case PF_BC5: case PF_BC6H: case PF_BC7:
	case PF_B8G8R8A8: case PF_R8G8B8A8:
	case PF_G8: case PF_R8G8:
		break;
	default:
		OutReason = FText::Format(LOCTEXT("BadFmt", "格式 {0} 不在支持白名单。"), FText::FromString(GPixelFormats[PF].Name));
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

UVirtualTextureCollection* SVirtualTextureCollectionManagerWidget::CreateVTCAsset(const FString& PackagePath, const FString& AssetName)
{
	FAssetToolsModule& ATM = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	IAssetTools& AssetTools = ATM.Get();
	FString UniqueName;
	FString UniquePkg;
	AssetTools.CreateUniqueAssetName(PackagePath / AssetName, TEXT(""), UniquePkg, UniqueName);
	UObject* NewObj = AssetTools.CreateAsset(UniqueName, PackagePath, UVirtualTextureCollection::StaticClass(), nullptr);
	return Cast<UVirtualTextureCollection>(NewObj);
}

void SVirtualTextureCollectionManagerWidget::AddTexturesToVTC(UVirtualTextureCollection* VTC, const TArray<UTexture*>& Textures)
{
	if (!VTC) return;
	VTC->Modify();
	for (UTexture* T : Textures)
	{
		if (T && !VTC->Textures.Contains(T))
		{
			VTC->Textures.Add(T);
		}
	}
	VTC->PostEditChange();
	VTC->MarkPackageDirty();
}

FReply SVirtualTextureCollectionManagerWidget::CreateNewVTC()
{
	const FString Path = TEXT("/Game/Art/Textures/VTC");
	if (!FPackageName::DoesPackageExist(Path))
	{
		IFileManager::Get().MakeDirectory(*FPackageName::LongPackageNameToFilename(Path), true);
	}
	UVirtualTextureCollection* VTC = CreateVTCAsset(Path, TEXT("VTC_New"));
	if (VTC)
	{
		FEditorFileUtils::PromptForCheckoutAndSave({VTC->GetOutermost()}, /*bCheckDirty*/ false, /*bPromptToSave*/ false);
		ScanVTCs();
	}
	return FReply::Handled();
}

FReply SVirtualTextureCollectionManagerWidget::CreateVTCFromContentBrowserSelection()
{
	TArray<UTexture*> Sel = CollectTexturesFromContentBrowser();
	if (Sel.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoSel", "Content Browser 里没有选中任何 Texture。请先在 Content Browser 里选中要打包的贴图。"));
		return FReply::Handled();
	}
	const FString Path = TEXT("/Game/Art/Textures/VTC");
	IFileManager::Get().MakeDirectory(*FPackageName::LongPackageNameToFilename(Path), true);
	UVirtualTextureCollection* VTC = CreateVTCAsset(Path, TEXT("VTC_FromSelection"));
	if (VTC)
	{
		AddTexturesToVTC(VTC, Sel);
		FEditorFileUtils::PromptForCheckoutAndSave({VTC->GetOutermost()}, false, false);
		ScanVTCs();
	}
	return FReply::Handled();
}

FReply SVirtualTextureCollectionManagerWidget::AddContentBrowserSelectionToCurrentVTC()
{
	if (!CurrentEntry.IsValid() || !CurrentEntry->VTC.IsValid())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoCurrent", "请先在左侧列表选中一个 VTC。"));
		return FReply::Handled();
	}
	TArray<UTexture*> Sel = CollectTexturesFromContentBrowser();
	if (Sel.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoSel2", "Content Browser 里没有选中任何 Texture。"));
		return FReply::Handled();
	}
	AddTexturesToVTC(CurrentEntry->VTC.Get(), Sel);
	FEditorFileUtils::PromptForCheckoutAndSave({CurrentEntry->VTC->GetOutermost()}, false, false);
	ScanVTCs();
	return FReply::Handled();
}

FReply SVirtualTextureCollectionManagerWidget::RemoveSelectedMembersFromCurrentVTC()
{
	if (!CurrentEntry.IsValid() || !CurrentEntry->VTC.IsValid() || SelectedMemberIndices.IsEmpty())
	{
		return FReply::Handled();
	}
	UVirtualTextureCollection* VTC = CurrentEntry->VTC.Get();

	// 从大到小移除，防止索引失效
	TArray<int32> Indices;
	for (const TSharedPtr<int32>& P : SelectedMemberIndices) { if (P.IsValid()) Indices.Add(*P); }
	Indices.Sort([](int32 A, int32 B) { return A > B; });

	VTC->Modify();
	for (int32 I : Indices)
	{
		if (VTC->Textures.IsValidIndex(I)) VTC->Textures.RemoveAt(I);
	}
	VTC->PostEditChange();
	VTC->MarkPackageDirty();
	FEditorFileUtils::PromptForCheckoutAndSave({VTC->GetOutermost()}, false, false);
	ScanVTCs();
	return FReply::Handled();
}

FReply SVirtualTextureCollectionManagerWidget::OpenCurrentVTC()
{
	if (CurrentEntry.IsValid() && CurrentEntry->VTC.IsValid() && GEditor)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(CurrentEntry->VTC.Get());
	}
	return FReply::Handled();
}

FReply SVirtualTextureCollectionManagerWidget::ValidateAll()
{
	for (const FVTCEntryPtr& E : AllEntries) { ValidateEntry(*E); }
	if (VTCListView.IsValid()) VTCListView->RequestListRefresh();
	return FReply::Handled();
}

FText SVirtualTextureCollectionManagerWidget::GetSummaryText() const
{
	int32 IssueTotal = 0;
	int32 MemberTotal = 0;
	int64 VRAM = 0;
	for (const FVTCEntryPtr& E : AllEntries)
	{
		IssueTotal += E->Issues.Num();
		MemberTotal += E->MemberCount;
		VRAM += E->TotalVRAMBytes;
	}
	return FText::Format(
		LOCTEXT("VTCSummary", "VTC {0} | 成员 {1} | 问题 {2} | 估算 VRAM {3} MB"),
		FText::AsNumber(AllEntries.Num()),
		FText::AsNumber(MemberTotal),
		FText::AsNumber(IssueTotal),
		FText::AsNumber(VRAM / 1048576));
}

FText SVirtualTextureCollectionManagerWidget::GetDetailHeaderText() const
{
	if (!CurrentEntry.IsValid()) return LOCTEXT("NoEntry", "在左侧选中一个 VTC 查看成员。");
	return FText::Format(LOCTEXT("Detail", "{0} — {1} 个成员，最大尺寸 {2}，运行时格式 {3}"),
		FText::FromString(CurrentEntry->AssetName),
		FText::AsNumber(CurrentEntry->MemberCount),
		FText::AsNumber(CurrentEntry->MaxSize),
		FText::FromString(CurrentEntry->RuntimeFormat));
}

FText SVirtualTextureCollectionManagerWidget::GetDetailIssuesText() const
{
	if (!CurrentEntry.IsValid()) return FText::GetEmpty();
	if (CurrentEntry->Issues.IsEmpty()) return LOCTEXT("NoIssues", "无问题。所有成员通过 VTC 兼容检查。");
	FString S;
	for (const FVTCMemberIssue& I : CurrentEntry->Issues)
	{
		S += FString::Printf(TEXT("• %s: %s\n"), *I.TextureName, *I.Message.ToString());
	}
	return FText::FromString(S);
}

#undef LOCTEXT_NAMESPACE
