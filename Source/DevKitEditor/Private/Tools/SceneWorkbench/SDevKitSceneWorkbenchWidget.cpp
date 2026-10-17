#include "Tools/SceneWorkbench/SDevKitSceneWorkbenchWidget.h"

#include "Tools/SceneWorkbench/SDevKitSceneRoomConfigWidget.h"
#include "Tools/Workflow/DevKitWorkbenchUI.h"
#include "Tools/MapCreator/DevKitMapCreatorService.h"
#include "LevelEnvironment/YogLevelEnvironment.h"
#include "StylizedLightingSettings.h"
#include "Data/RoomDataAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Editor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "EngineUtils.h"
#include "Factories/DataAssetFactory.h"
#include "FileHelpers.h"
#include "IDetailTreeNode.h"
#include "IPropertyRowGenerator.h"
#include "PropertyHandle.h"
#include "PropertyEditorModule.h"
#include "ISinglePropertyView.h"
#include "PropertyCustomizationHelpers.h"
#include "Misc/PackageName.h"
#include "Misc/MessageDialog.h"
#include "Framework/Docking/TabManager.h"
#include "HAL/IConsoleManager.h"
#include "ScopedTransaction.h"
#include "Containers/Ticker.h"
#include "UObject/UnrealType.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

namespace
{
    const FLinearColor Blue(.12f, .55f, .9f), Green(.25f, .75f, .5f), Amber(.9f, .6f, .2f);
    FAutoConsoleCommand OpenSceneWorkbenchCommand(TEXT("DevKit.SceneWorkbench.Open"), TEXT("Open the scene asset/configuration workbench."),
        FConsoleCommandDelegate::CreateLambda([](){FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("DevKitSceneWorkbench")));}));

    TSharedRef<SWidget> Text(const FString& Value, FSlateColor Color = FSlateColor::UseForeground())
    {
        return SNew(STextBlock).Text(FText::FromString(Value)).ColorAndOpacity(Color).AutoWrapText(true);
    }

    TSharedRef<SWidget> Card(const FString& Title, const FString& Hint, TSharedRef<SWidget> Body, FLinearColor Accent = Blue)
    {
        return DevKitWorkbenchUI::MakeSection(FText::FromString(Title), Body, false,
            FText::FromString(Hint), Accent);
    }

    TSharedRef<SWidget> Button(const FString& Label, TFunction<FReply()> Action, bool bPrimary = false)
    {
        return SNew(SButton).ButtonStyle(FAppStyle::Get(), bPrimary ? "PrimaryButton" : "Button")
            .ContentPadding(FMargin(10, 5)).Text(FText::FromString(Label)).OnClicked_Lambda(MoveTemp(Action));
    }

    TSharedRef<SWidget> Field(const FString& Label, TSharedRef<SWidget> Value)
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(.35f).VAlign(VAlign_Center).Padding(0, 4)[Text(Label)]
            + SHorizontalBox::Slot().FillWidth(.65f).Padding(4)[Value];
    }

    TSharedPtr<IPropertyHandle> FindProperty(const TArray<TSharedRef<IDetailTreeNode>>& Nodes, FName Name)
    {
        for (const TSharedRef<IDetailTreeNode>& Node : Nodes)
        {
            const TSharedPtr<IPropertyHandle> Handle = Node->CreatePropertyHandle();
            if (Handle && Handle->GetProperty() && Handle->GetProperty()->GetFName() == Name) return Handle;
            TArray<TSharedRef<IDetailTreeNode>> Children;
            Node->GetChildren(Children);
            if (TSharedPtr<IPropertyHandle> Found = FindProperty(Children, Name)) return Found;
        }
        return nullptr;
    }

    TSharedRef<SWidget> Choice(TArray<FString> Choices, TFunction<FString()> Get, TFunction<void(FString)> Set)
    {
        TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
        for (const FString& Value : Choices)
            Row->AddSlot().AutoWidth().Padding(0, 0, 4, 0)
            [SNew(SButton).ButtonColorAndOpacity_Lambda([Get, Value](){ return Get() == Value ? Blue : FLinearColor(.18f, .18f, .18f); })
                .Text(FText::FromString(Value)).OnClicked_Lambda([Set, Value](){Set(Value); return FReply::Handled();})];
        return Row;
    }

    AYogLevelEnvironment* FindEditablePersistentEnvironment(UWorld* World)
    {
        AYogLevelEnvironment* Best = nullptr;
        if (World) for (TActorIterator<AYogLevelEnvironment> It(World); It; ++It)
        {
            AYogLevelEnvironment* Actor = *It;
            if (Actor->GetLevel() != World->PersistentLevel || !IsValid(Actor->EnvironmentComponent)
                || Actor->IsActorBeingDestroyed()) continue;
            if (!Best || Actor->EnvironmentComponent->Priority > Best->EnvironmentComponent->Priority
                || (Actor->EnvironmentComponent->Priority == Best->EnvironmentComponent->Priority
                    && Actor->GetPathName() < Best->GetPathName())) Best = Actor;
        }
        return Best;
    }
}

bool SDevKitSceneWorkbenchWidget::DoesRoomMatchMap(const URoomDataAsset* Room, const FAssetData& Map, bool bUniqueLegacyName)
{
    if (!Room || !Map.IsValid()) return false;
    if (!Room->LevelAsset.IsNull()) return Room->LevelAsset.ToSoftObjectPath().GetLongPackageName() == Map.PackageName.ToString();
    return bUniqueLegacyName && Room->RoomName == Map.AssetName;
}

const FString& SDevKitSceneWorkbenchWidget::GetMapDataRoot()
{
    static const FString Root(TEXT("/Game/Art/Map/Map_Data"));
    return Root;
}

bool SDevKitSceneWorkbenchWidget::IsPrimaryMapPackage(const FString& PackageName)
{
    if (!FPackageName::IsValidLongPackageName(PackageName)) return false;
    const FString Folder = FPackageName::GetLongPackagePath(PackageName);
    const FString& Root = GetMapDataRoot();
    if (!Folder.Equals(Root, ESearchCase::IgnoreCase) && !Folder.StartsWith(Root + TEXT("/"), ESearchCase::IgnoreCase)) return false;

    // Existing packages include legacy _Bake, _VFX and mixed-case _GamePlay.
    // InitialRoom is a valid persistent map, so do not require an L1_ prefix
    // or an asset name that exactly matches its enclosing folder.
    const FString Name = FPackageName::GetLongPackageAssetName(PackageName);
    for (const TCHAR* Suffix : {TEXT("_Art"), TEXT("_Batched"), TEXT("_Bake"), TEXT("_DataBake"), TEXT("_Gameplay"), TEXT("_Light"), TEXT("_PLA"), TEXT("_VFX")})
        if (Name.EndsWith(Suffix, ESearchCase::IgnoreCase)) return false;
    return true;
}

void SDevKitSceneWorkbenchWidget::Construct(const FArguments& InArgs)
{
    GlobalSettingsObject = GetMutableDefault<UStylizedLightingSettings>();
    GlobalEnvironmentChangedHandle = GlobalSettingsObject->OnSettingChanged().AddSP(this,
        &SDevKitSceneWorkbenchWidget::OnGlobalEnvironmentChanged);
    RoomForm = SNew(SDevKitSceneRoomConfigWidget);
    ChildSlot[SNew(SSplitter)
        + SSplitter::Slot().Value(.28f).MinSize(260)
        [SNew(SBorder).BorderImage(FAppStyle::GetBrush("Brushes.Recessed")).Padding(10)
            [SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [DevKitWorkbenchUI::MakeSection(FText::FromString(TEXT("关卡资源 / 筛选与创建")),
                    SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
                [SNew(SSearchBox).HintText(FText::FromString(TEXT("搜索名称、路径或房间")))
                    .OnTextChanged_Lambda([this](const FText& Value){ Search = Value.ToString(); FilterAssets(); })]
                + SVerticalBox::Slot().AutoHeight()[SNew(SEditableTextBox).IsReadOnly(true).Text(FText::FromString(GetMapDataRoot()))
                    .ToolTipText(FText::FromString(TEXT("固定扫描目录：只列出主关卡。Art / Batched / Bake / DataBake / Gameplay / Light / PLA / VFX 子关卡仍在右侧资源页管理。")))]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
                [Choice({TEXT("全部"), TEXT("L1"), TEXT("L2"), TEXT("L3"), TEXT("未配置玩法")},
                    [this](){return LayerFilter.IsEmpty() ? FString(TEXT("全部")) : LayerFilter;},
                    [this](FString Value){LayerFilter = Value == TEXT("全部") ? FString() : Value; FilterAssets();})]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
                [SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1)[Button(TEXT("刷新"), [this](){RefreshAssets(); return FReply::Handled();})]
                    + SHorizontalBox::Slot().FillWidth(1).Padding(6, 0, 0, 0)[Button(TEXT("＋ 新建地图包"), [this](){if (ConfirmDiscardEnvironmentDraft()) {bCreatingMap = true; RebuildRightPanel();} return FReply::Handled();}, true)]], false, FText::GetEmpty(), Blue)]
                + SVerticalBox::Slot().FillHeight(1).Padding(0, 8)
                [SAssignNew(AssetList, SListView<FRow>).ListItemsSource(&VisibleRows).SelectionMode(ESelectionMode::Single)
                    .OnGenerateRow(this, &SDevKitSceneWorkbenchWidget::MakeAssetRow).OnSelectionChanged(this, &SDevKitSceneWorkbenchWidget::SelectAsset)]
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this](){return FText::FromString(FString::Printf(TEXT("显示 %d / %d · 房间数据 %d"), VisibleRows.Num(), AllRows.Num(), LoadedRooms.Num()));})
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())]]]
        + SSplitter::Slot().Value(.72f)
        [SNew(SVerticalBox)
            + SVerticalBox::Slot().FillHeight(1)[SAssignNew(RightPanel, SBox)]
            + SVerticalBox::Slot().AutoHeight().Padding(12, 8)[SNew(STextBlock).Text_Lambda([this](){return Status;}).AutoWrapText(true).ColorAndOpacity(Amber)]]];
    RefreshAssets();
}

SDevKitSceneWorkbenchWidget::~SDevKitSceneWorkbenchWidget()
{
    if (GlobalSettingsObject.IsValid())
        GlobalSettingsObject->OnSettingChanged().Remove(GlobalEnvironmentChangedHandle);
}

void SDevKitSceneWorkbenchWidget::OnGlobalEnvironmentChanged(UObject* Object, FPropertyChangedEvent&)
{
    if (Object && Object->IsA<UStylizedLightingSettings>() && EnvironmentDraft.IsValid()
        && !EnvironmentDraft->bOverrideGlobalSettings) QueueEnvironmentRefresh();
}

void SDevKitSceneWorkbenchWidget::RefreshAssets()
{
    FString Previous = Selected ? Selected->Path : FString();
    if (Previous.IsEmpty() && GEditor && GEditor->GetEditorWorldContext().World())
        Previous = GEditor->GetEditorWorldContext().World()->GetPackage()->GetName();
    AllRows.Reset();
    LoadedRooms.Reset();
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    TArray<FAssetData> Maps, Rooms;
    FARFilter Filter;
    Filter.ClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(*GetMapDataRoot()));
    Filter.bRecursivePaths = true;
    Registry.GetAssets(Filter, Maps);
    Maps.RemoveAll([](const FAssetData& Map){return !IsPrimaryMapPackage(Map.PackageName.ToString());});
    Registry.GetAssetsByClass(URoomDataAsset::StaticClass()->GetClassPathName(), Rooms, true);
    TMap<FName, int32> NameCounts;
    for (const FAssetData& Map : Maps) ++NameCounts.FindOrAdd(Map.AssetName);
    for (const FAssetData& Asset : Rooms)
        if (URoomDataAsset* Room = Cast<URoomDataAsset>(Asset.GetAsset())) LoadedRooms.Emplace(Room);
    for (const FAssetData& Map : Maps)
    {
        FRow Row = MakeShared<FDevKitSceneAssetRow>();
        Row->Map = Map; Row->Name = Map.AssetName.ToString(); Row->Path = Map.PackageName.ToString();
        for (const auto& Room : LoadedRooms)
            if (DoesRoomMatchMap(Room.Get(), Map, NameCounts.FindRef(Map.AssetName) == 1)) Row->Rooms.Add(Room.Get());
        AllRows.Add(Row);
    }
    AllRows.Sort([](const FRow& A, const FRow& B){ return A->Path < B->Path; });
    FilterAssets();
    for (const FRow& Row : VisibleRows)
        if (Row->Path == Previous) {AssetList->SetSelection(Row); return;}
    if (VisibleRows.Num()) AssetList->SetSelection(VisibleRows[0]);
    else {Selected.Reset(); SelectedRoom.Reset(); RebuildRightPanel();}
}

void SDevKitSceneWorkbenchWidget::FilterAssets()
{
    VisibleRows.Reset();
    for (const FRow& Row : AllRows)
    {
        if (LayerFilter == TEXT("未配置玩法") && !Row->Rooms.IsEmpty()) continue;
        if (!LayerFilter.IsEmpty() && LayerFilter != TEXT("未配置玩法") && !Row->Name.StartsWith(LayerFilter) && !Row->Path.Contains(TEXT("/") + LayerFilter)) continue;
        bool bMatch = Search.IsEmpty() || Row->Name.Contains(Search) || Row->Path.Contains(Search);
        for (const auto& Room : Row->Rooms) if (Room.IsValid()) bMatch |= Room->DisplayName.ToString().Contains(Search) || Room->GetName().Contains(Search);
        if (bMatch) VisibleRows.Add(Row);
    }
    if (AssetList) AssetList->RequestListRefresh();
}

TSharedRef<ITableRow> SDevKitSceneWorkbenchWidget::MakeAssetRow(FRow Row, const TSharedRef<STableViewBase>& Owner)
{
    return SNew(STableRow<FRow>, Owner).Padding(8)
        [SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Row->Name)).Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 3)[Text(Row->Rooms.IsEmpty() ? TEXT("主关卡 · 未配置玩法") : FString::Printf(TEXT("主关卡 · %d 个玩法变体"), Row->Rooms.Num()), Row->Rooms.IsEmpty() ? Amber : Green)]
            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Row->Path))
                .ToolTipText(FText::FromString(Row->Path)).ColorAndOpacity(FSlateColor::UseSubduedForeground()).Font(FAppStyle::GetFontStyle("SmallText")).AutoWrapText(true)]];
}

void SDevKitSceneWorkbenchWidget::SelectAsset(FRow Row, ESelectInfo::Type)
{
    if (Row == Selected) return;
    const bool bSameMap = Row && Selected && Row->Path == Selected->Path;
    if (!bSameMap && !ConfirmDiscardEnvironmentDraft()) {if (AssetList && Selected) AssetList->SetSelection(Selected); return;}
    const TWeakObjectPtr<URoomDataAsset> PreviousRoom = SelectedRoom;
    Selected = Row; bCreatingMap = false; RoomVariants.Reset();
    if (Selected) for (const auto& Room : Selected->Rooms) RoomVariants.Add(MakeShared<TWeakObjectPtr<URoomDataAsset>>(Room));
    SelectedRoom = RoomVariants.Num() ? *RoomVariants[0] : TWeakObjectPtr<URoomDataAsset>();
    if (bSameMap && PreviousRoom.IsValid() && Selected->Rooms.Contains(PreviousRoom)) SelectedRoom = PreviousRoom;
    RoomForm->SetRoom(SelectedRoom.Get());
    SublevelFolder = Selected && Selected->Map.IsValid() ? Selected->Map.PackagePath.ToString() : CreateRoot;
    if (!bSameMap || (!bEnvironmentDraftDirty && !bNativeStyleDraftDirty)) LoadEnvironmentDraft();
    RebuildRightPanel();
}

void SDevKitSceneWorkbenchWidget::SelectRoom(TSharedPtr<TWeakObjectPtr<URoomDataAsset>> Room, ESelectInfo::Type)
{
    SelectedRoom = Room ? *Room : TWeakObjectPtr<URoomDataAsset>();
    RoomForm->SetRoom(SelectedRoom.Get()); RebuildRightPanel();
}

FString SDevKitSceneWorkbenchWidget::GetSelectedPackage() const
{
    return Selected && Selected->Map.IsValid() ? Selected->Map.PackageName.ToString() : FString();
}

UWorld* SDevKitSceneWorkbenchWidget::GetSelectedEditorWorld() const
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    return World && World->GetPackage()->GetName() == GetSelectedPackage() ? World : nullptr;
}

FReply SDevKitSceneWorkbenchWidget::OpenSelectedMap()
{
    const FString Package = GetSelectedPackage();
    if (Package.IsEmpty()) {Status = FText::FromString(TEXT("该房间尚未关联地图，请先选择或关联关卡资产。")); return FReply::Handled();}
    if (GEditor && GEditor->IsPlaySessionInProgress()) {Status = FText::FromString(TEXT("请先停止 PIE。")); return FReply::Handled();}
    const bool bAlreadyOpen = GetSelectedEditorWorld() != nullptr;
    if (!bAlreadyOpen)
    {
        if (!FDevKitMapCreatorService::SaveCurrentEditorWorld()) return FReply::Handled();
        if (!FEditorFileUtils::LoadMap(FPackageName::LongPackageNameToFilename(Package, FPackageName::GetMapPackageExtension())))
        {Status = FText::FromString(TEXT("打开关卡失败：") + Package); return FReply::Handled();}
    }
    Status = FText::FromString(TEXT("已打开：") + Package);
    if (!bAlreadyOpen) LoadEnvironmentDraft();
    RebuildRightPanel(); return FReply::Handled();
}

FReply SDevKitSceneWorkbenchWidget::SaveSelected()
{
    if (bNativeStyleDraftDirty)
    {
        ApplyNativeStyle();
        if (bNativeStyleDraftDirty) {Status = FText::FromString(TEXT("请先打开所选关卡并停止 PIE，再保存关卡调色覆盖。")); return FReply::Handled();}
    }
    if (bEnvironmentDraftDirty)
    {
        if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(TEXT("环境备忘尚未写入。是否先写入备忘，再保存所选关卡与房间数据？"))) != EAppReturnType::Yes) return FReply::Handled();
        ApplyEnvironment();
        if (bEnvironmentDraftDirty) {Status = FText::FromString(TEXT("环境备忘尚未写入，请先打开所选关卡并停止 PIE。")); return FReply::Handled();}
    }
    TArray<UPackage*> Packages;
    if (SelectedRoom.IsValid()) Packages.AddUnique(SelectedRoom->GetPackage());
    if (UWorld* World = GetSelectedEditorWorld())
    {
        Packages.AddUnique(World->GetPackage());
        if (UPackage* External = World->GetWorldSettings()->GetExternalPackage()) Packages.AddUnique(External);
        for (TActorIterator<AYogLevelEnvironment> It(World); It; ++It)
            if (It->GetLevel() == World->PersistentLevel)
                if (UPackage* External = It->GetExternalPackage()) Packages.AddUnique(External);
        for (TActorIterator<AYogRenderStyleActor> It(World); It; ++It)
            if (It->GetLevel() == World->PersistentLevel)
                if (UPackage* External = It->GetExternalPackage()) Packages.AddUnique(External);
    }
    if (Packages.Num())
    {
        const auto Result = FEditorFileUtils::PromptForCheckoutAndSave(Packages, true, false);
        Status = FText::FromString(Result == FEditorFileUtils::PR_Success ? TEXT("已保存选中的房间数据与主关卡；未保存其他资产。") : TEXT("保存未完成，请检查取消/签出状态。"));
    }
    return FReply::Handled();
}

void SDevKitSceneWorkbenchWidget::RebuildRightPanel()
{
    if (bCreatingMap) {RightPanel->SetContent(MakeNewMapPanel()); return;}
    if (!Selected) {RightPanel->SetContent(Card(TEXT("选择关卡开始配置"), TEXT("左侧选择地图；也可以新建地图包。旧房间资产不会被迁移、删除或自动关联。"), SNullWidget::NullWidget)); return;}
    TSharedRef<SVerticalBox> Page = SNew(SVerticalBox);
    Page->AddSlot().AutoHeight().Padding(12)
    [SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1)[SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Selected->Name)).Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)[Text(Selected->Path, FSlateColor::UseSubduedForeground())]]
        + SHorizontalBox::Slot().AutoWidth().Padding(4, 0)[Button(TEXT("打开关卡"), [this](){return OpenSelectedMap();}, true)]
        + SHorizontalBox::Slot().AutoWidth().Padding(4, 0)[Button(TEXT("保存所选配置"), [this](){return SaveSelected();})]
        + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("定位资产"), [this](){FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().SyncBrowserToAssets({Selected->Map}); return FReply::Handled();})]];
    Page->AddSlot().AutoHeight().Padding(12, 0)
    [SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)[Text(TEXT("玩法变体"), Green)]
        + SHorizontalBox::Slot().FillWidth(1)
        [SNew(SComboBox<TSharedPtr<TWeakObjectPtr<URoomDataAsset>>>).OptionsSource(&RoomVariants)
            .OnGenerateWidget_Lambda([](TSharedPtr<TWeakObjectPtr<URoomDataAsset>> Item){return Text(Item && Item->IsValid() ? Item->Get()->GetName() : TEXT("无房间配置"));})
            .OnSelectionChanged(this, &SDevKitSceneWorkbenchWidget::SelectRoom)
            [SNew(STextBlock).Text_Lambda([this](){return FText::FromString(SelectedRoom.IsValid() ? SelectedRoom->GetName() : TEXT("未配置玩法（可纯美术关卡）"));})]]
        + SHorizontalBox::Slot().AutoWidth().Padding(6, 0)[Button(TEXT("＋ 新建房间数据"), [this](){return CreateRoom();})]];
    Page->AddSlot().AutoHeight().Padding(12, 6, 12, 0)
    [SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)[Text(TEXT("关联已有数据"), FSlateColor::UseSubduedForeground())]
        + SHorizontalBox::Slot().FillWidth(1)
        [SNew(SObjectPropertyEntryBox).AllowedClass(URoomDataAsset::StaticClass()).DisplayThumbnail(false)
            .ObjectPath_Lambda([this](){return SelectedRoom.IsValid() ? SelectedRoom->GetPathName() : FString();})
            .OnObjectChanged_Lambda([this](const FAssetData& Asset){SelectedRoom = Cast<URoomDataAsset>(Asset.GetAsset()); RoomForm->SetRoom(SelectedRoom.Get()); RebuildRightPanel();})]
        + SHorizontalBox::Slot().AutoWidth().Padding(6, 0)[Button(TEXT("关联所选房间"), [this](){return AssociateRoom();})]];
    TSharedRef<SHorizontalBox> Tabs = SNew(SHorizontalBox);
    const TArray<FString> Labels{TEXT("关卡默认"), TEXT("游戏玩法"), TEXT("美术环境"), TEXT("子关卡资源")};
    for (int32 Index = 0; Index < Labels.Num(); ++Index)
        Tabs->AddSlot().FillWidth(1).Padding(3)
            [SNew(SButton).ButtonStyle(FAppStyle::Get(), ActivePage == Index ? "PrimaryButton" : "Button")
                .ContentPadding(FMargin(8, 8)).Text(FText::FromString(Labels[Index]))
                .OnClicked_Lambda([this, Index](){ActivePage = Index; RebuildRightPanel(); return FReply::Handled();})];
    Page->AddSlot().AutoHeight().Padding(10, 12)[Tabs];
    TSharedRef<SWidget> Body = ActivePage == 0 ? RoomForm->MakeDefaultsPanel() : ActivePage == 1 ? RoomForm->MakeGameplayPanel() : ActivePage == 2 ? MakeEnvironmentPanel() : MakeSublevelsPanel();
    if (ActivePage == 0)
    {
        TSharedRef<SVerticalBox> Defaults = SNew(SVerticalBox);
        TSharedRef<SVerticalBox> WorldFields = SNew(SVerticalBox);
        if (UWorld* World = GetSelectedEditorWorld())
        {
            auto& Module = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
            for (const auto& Item : TArray<TPair<FName, FString>>{{TEXT("DefaultGameMode"), TEXT("关卡 GameMode")}, {TEXT("KillZ"), TEXT("Kill Z")}, {TEXT("bGlobalGravitySet"), TEXT("覆盖重力")}, {TEXT("GlobalGravityZ"), TEXT("关卡重力 Z")}})
            {
                FSinglePropertyParams Params; Params.NamePlacement = EPropertyNamePlacement::Hidden;
                if (auto Property = Module.CreateSingleProperty(World->GetWorldSettings(), Item.Key, Params))
                    WorldFields->AddSlot().AutoHeight()[Field(Item.Value, Property.ToSharedRef())];
            }
        }
        else WorldFields->AddSlot().AutoHeight()[Button(TEXT("打开关卡以编辑世界默认设置"), [this](){return OpenSelectedMap();})];
        WorldFields->AddSlot().AutoHeight().Padding(0, 10)[Button(TEXT("全局 Campaign 流程（保留原入口）"), [](){FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("DevKitLevelDataWorkbench"))); return FReply::Handled();})];
        Defaults->AddSlot().AutoHeight()[Card(TEXT("关卡世界默认设置"), TEXT("只编辑所选关卡，不写项目全局配置。房间玩法变体保留独立 RoomData。"), WorldFields)];
        Defaults->AddSlot().FillHeight(1).Padding(0, 10)[Body]; Body = Defaults;
    }
    if (ActivePage < 2) Page->AddSlot().FillHeight(1).Padding(12, 0)[Body];
    else Page->AddSlot().FillHeight(1).Padding(12, 0)[SNew(SScrollBox) + SScrollBox::Slot()[Body]];
    RightPanel->SetContent(Page);
}

FReply SDevKitSceneWorkbenchWidget::AssociateRoom()
{
    if (!SelectedRoom.IsValid() || GetSelectedPackage().IsEmpty()) {Status = FText::FromString(TEXT("先选关卡资产和需要关联的 RoomData。")); return FReply::Handled();}
    const FScopedTransaction Transaction(FText::FromString(TEXT("关联关卡与房间配置")));
    SelectedRoom->Modify();
    SelectedRoom->LevelAsset = TSoftObjectPtr<UWorld>(Selected->Map.GetSoftObjectPath());
    SelectedRoom->RoomName = Selected->Map.AssetName;
    SelectedRoom->MarkPackageDirty();
    Status = FText::FromString(TEXT("已关联并同步 RoomName；点击保存后写入资产。"));
    RefreshAssets(); return FReply::Handled();
}

FReply SDevKitSceneWorkbenchWidget::CreateRoom()
{
    if (GetSelectedPackage().IsEmpty()) {Status = FText::FromString(TEXT("请先选择一张地图。")); return FReply::Handled();}
    IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    FString Package, Name;
    Tools.CreateUniqueAssetName(Selected->Map.PackagePath.ToString() / (TEXT("DA_Room_") + Selected->Name), TEXT(""), Package, Name);
    UDataAssetFactory* Factory = NewObject<UDataAssetFactory>(); Factory->DataAssetClass = URoomDataAsset::StaticClass();
    URoomDataAsset* Room = Cast<URoomDataAsset>(Tools.CreateAsset(Name, FPackageName::GetLongPackagePath(Package), URoomDataAsset::StaticClass(), Factory));
    if (Room)
    {
        Room->LevelAsset = TSoftObjectPtr<UWorld>(Selected->Map.GetSoftObjectPath()); Room->RoomName = Selected->Map.AssetName;
        Room->DisplayName = FText::FromString(Selected->Name); Room->MarkPackageDirty();
        RefreshAssets(); SelectedRoom = Room; RoomForm->SetRoom(Room); RebuildRightPanel();
        Status = FText::FromString(TEXT("已新建玩法变体（未保存）；不会自动加入 Campaign，请按玩法流程配置。"));
    }
    return FReply::Handled();
}

TSharedRef<SWidget> SDevKitSceneWorkbenchWidget::MakeNewMapPanel()
{
    TSharedRef<SVerticalBox> Form = SNew(SVerticalBox);
    Form->AddSlot().AutoHeight()[Field(TEXT("目标根目录"), SNew(SEditableTextBox).Text(FText::FromString(CreateRoot)).OnTextChanged_Lambda([this](const FText& Value){CreateRoot = Value.ToString();}))];
    Form->AddSlot().AutoHeight()[Field(TEXT("大关卡"), Choice({TEXT("L1"), TEXT("L2"), TEXT("L3")}, [this](){return NewLayer;}, [this](FString Value){NewLayer = Value;}))];
    Form->AddSlot().AutoHeight()[Field(TEXT("地图类型"), Choice(FDevKitMapCreatorService::GetLevelTypes(), [this](){return NewType;}, [this](FString Value){NewType = Value;}))];
    Form->AddSlot().AutoHeight()[Field(TEXT("名称后缀"), SNew(SEditableTextBox).HintText(FText::FromString(TEXT("Corridor_S_Goth"))).Text(FText::FromString(NewSuffix)).OnTextChanged_Lambda([this](const FText& Value){NewSuffix = Value.ToString();}))];
    Form->AddSlot().AutoHeight().Padding(0, 10)[SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(Green).Text_Lambda([this]()
    {
        FText Error; const auto Paths = FDevKitMapCreatorService::BuildPaths({CreateRoot, NewLayer, NewType, NewSuffix}, Error);
        return Paths.IsSet() ? FText::FromString(TEXT("创建预览：\n") + Paths->TargetFolder + TEXT("\n主关卡 + Art / Batched / DataBake / Gameplay / Light / PLA")) : Error;
    })];
    Form->AddSlot().AutoHeight()[SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("创建地图包并选择"), [this]()
        {
            FText Error;
            const auto Paths = FDevKitMapCreatorService::BuildPaths({CreateRoot, NewLayer, NewType, NewSuffix}, Error);
            if (!Paths.IsSet()) {Status = Error; return FReply::Handled();}
            if (!IsPrimaryMapPackage(Paths->PersistentMap))
            {
                Status = FText::FromString(TEXT("请在 ") + GetMapDataRoot() + TEXT(" 或其子目录创建主关卡；名称不能使用子关卡后缀。其它目录仍可通过高级地图创建器管理。"));
                return FReply::Handled();
            }
            const auto Result = FDevKitMapCreatorService::CreateLevelStack({CreateRoot, NewLayer, NewType, NewSuffix}); Status = Result.Message;
            if (Result.bSuccess)
            {
                bCreatingMap = false; RefreshAssets();
                for (const FRow& Row : VisibleRows) if (Row->Path == Result.Paths.PersistentMap) {AssetList->SetSelection(Row); break;}
            }
            return FReply::Handled();
        }, true)]
        + SHorizontalBox::Slot().AutoWidth().Padding(8, 0)[Button(TEXT("取消"), [this](){bCreatingMap = false; LoadEnvironmentDraft(); RebuildRightPanel(); return FReply::Handled();})]];
    return SNew(SScrollBox) + SScrollBox::Slot().Padding(12)[Card(TEXT("新建场景地图包"), TEXT("沿用项目命名与模板。只创建新资源，不覆盖同名地图；创建后继续在当前工作台配置玩法与环境。"), Form)];
}

TSharedRef<SWidget> SDevKitSceneWorkbenchWidget::MakeSublevelsPanel()
{
    UWorld* World = GetSelectedEditorWorld();
    if (!World) return Card(TEXT("子关卡资源"), TEXT("先打开左侧选中的主关卡，再查看真实流送引用和创建子关卡。"), Button(TEXT("打开所选关卡"), [this](){return OpenSelectedMap();}, true));
    if (World->IsPartitionedWorld()) return Card(TEXT("World Partition 关卡"), TEXT("此工具管理传统流送子关卡；不会将 WP 地图强制转换为流送地图。"), SNullWidget::NullWidget, Amber);
    TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
    Body->AddSlot().AutoHeight()[Field(TEXT("子关卡目标目录"), SNew(SEditableTextBox).Text(FText::FromString(SublevelFolder)).OnTextChanged_Lambda([this](const FText& Value){SublevelFolder = Value.ToString();}))];
    Body->AddSlot().AutoHeight().Padding(0, 6)[Field(TEXT("灯光模板所属关卡"), Choice({TEXT("L1"), TEXT("L2"), TEXT("L3")}, [this](){return NewLayer;}, [this](FString Value){NewLayer = Value;}))];
    for (const FString& Suffix : FDevKitMapCreatorService::GetSublevelSuffixes())
    {
        const FString Planned = SublevelFolder / (Selected->Name + TEXT("_") + Suffix);
        ULevelStreaming* Referenced = nullptr;
        for (ULevelStreaming* Streaming : World->GetStreamingLevels())
            if (Streaming && FPackageName::GetLongPackageAssetName(Streaming->GetWorldAssetPackageName()).EndsWith(TEXT("_") + Suffix)) {Referenced = Streaming; break;}
        const FString Reference = Referenced ? Referenced->GetWorldAssetPackageName() : FString();
        Body->AddSlot().AutoHeight().Padding(0, 3)
        [SNew(SBorder).BorderImage(FAppStyle::GetBrush("Brushes.Recessed")).Padding(8)
            [SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(90)[Text(Suffix, Blue)]]
                + SHorizontalBox::Slot().FillWidth(1).Padding(8, 0)[Text(Referenced ? TEXT("已引用 · ") + Reference : TEXT("未引用 · ") + Planned, Referenced ? Green : Amber)]
                + SHorizontalBox::Slot().AutoWidth()[Button(Referenced ? TEXT("定位") : TEXT("创建并引用"), [this, Suffix, Reference]()
                {
                    if (!Reference.IsEmpty())
                    {
                        const FAssetData Asset = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().GetAssetByObjectPath(FSoftObjectPath(Reference + TEXT(".") + FPackageName::GetLongPackageAssetName(Reference)));
                        FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get().SyncBrowserToAssets({Asset});
                    }
                    else
                    {
                        const auto Result = FDevKitMapCreatorService::CreateAndAttachSublevel(GetSelectedEditorWorld(), {SublevelFolder, Suffix, NewLayer});
                        Status = Result.Message; RebuildRightPanel();
                    }
                    return FReply::Handled();
                })]]];
    }
    Body->AddSlot().AutoHeight().Padding(0, 12)[Text(TEXT("主关卡的全部流送引用（包含非标准名称）"), Green)];
    for (ULevelStreaming* Streaming : World->GetStreamingLevels())
        if (Streaming) Body->AddSlot().AutoHeight().Padding(0, 3)[Text(Streaming->GetWorldAssetPackageName(), FSlateColor::UseSubduedForeground())];
    Body->AddSlot().AutoHeight().Padding(0, 12)[Field(TEXT("引用已有子关卡"), SNew(SObjectPropertyEntryBox).AllowedClass(UWorld::StaticClass()).DisplayThumbnail(false)
        .ObjectPath_Lambda([this](){return ExistingSublevel;}).OnObjectChanged_Lambda([this](const FAssetData& Asset){ExistingSublevel = Asset.GetSoftObjectPath().ToString();}))];
    Body->AddSlot().AutoHeight()[Button(TEXT("添加到主关卡"), [this]()
    {
        const auto Result = FDevKitMapCreatorService::AttachExistingSublevel(GetSelectedEditorWorld(), FSoftObjectPath(ExistingSublevel).GetLongPackageName());
        Status = Result.Message; RebuildRightPanel(); return FReply::Handled();
    })];
    return Card(TEXT("子关卡资源与地图包"), TEXT("Art、Batched、DataBake 等从这里创建并写入主关卡的流送引用。创建前保存主关卡；不会覆盖已有文件。"), Body);
}

void SDevKitSceneWorkbenchWidget::LoadEnvironmentDraft()
{
    bEnvironmentDraftDirty = false;
    bNativeStyleDraftDirty = false;
    bOverrideNativeStyleDraft = false;
    NativeStylePriorityDraft = 0;
    NativeStyleActor.Reset();
    NativeStyleDraft = GetDefault<UYogRenderStyleSettings>()->Style;
    // The project master is independent from a future explicit level override.
    NativeStyleDraft.bEnabled = true;
    EnvironmentGenerator.Reset(); EnvironmentValuesGenerator.Reset();
    EnvironmentDraft.Reset(); EnvironmentValues.Reset(); EnvironmentEditedActor.Reset();
    UWorld* World = GetSelectedEditorWorld(); if (!World) return;
    for (TActorIterator<AYogRenderStyleActor> It(World); It; ++It)
    {
        if (It->GetLevel() != World->PersistentLevel || !It->StyleComponent) continue;
        if (NativeStyleActor.IsValid())
        {
            const UYogRenderStyleComponent* Existing = NativeStyleActor->StyleComponent;
            const UYogRenderStyleComponent* Candidate = It->StyleComponent;
            if (Existing->bOverrideGlobalStyle && !Candidate->bOverrideGlobalStyle) continue;
            if (Existing->bOverrideGlobalStyle == Candidate->bOverrideGlobalStyle
                && (Existing->Priority > Candidate->Priority
                    || (Existing->Priority == Candidate->Priority && Existing->GetPathName() < Candidate->GetPathName()))) continue;
        }
        NativeStyleActor = *It;
    }
    if (NativeStyleActor.IsValid())
    {
        NativeStyleDraft = NativeStyleActor->StyleComponent->Style;
        bOverrideNativeStyleDraft = NativeStyleActor->StyleComponent->bOverrideGlobalStyle;
        NativeStylePriorityDraft = NativeStyleActor->StyleComponent->Priority;
    }
    EnvironmentDraft.Reset(NewObject<UYogLevelEnvironmentComponent>(GetTransientPackage(), NAME_None, RF_Transactional));
    EnvironmentValues.Reset(NewObject<UYogLevelEnvironmentComponent>(GetTransientPackage(), NAME_None, RF_Transactional));
    if (AYogLevelEnvironment* Actor = FindEditablePersistentEnvironment(World))
    {
        EnvironmentEditedActor = Actor;
        UYogLevelEnvironmentComponent* Source = Actor->EnvironmentComponent.Get();
        EnvironmentDraft->Profile = Source->Profile; EnvironmentDraft->Settings = Source->Settings;
        EnvironmentDraft->bOverrideGlobalSettings = Source->bOverrideGlobalSettings;
        EnvironmentDraft->bOverrideProfile = Source->bOverrideProfile; EnvironmentDraft->bEnabled = Source->bEnabled;
        EnvironmentDraft->Notes = Source->Notes;
        EnvironmentDraft->Priority = Source->Priority;
    }
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    EnvironmentGenerator = PropertyModule.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
    EnvironmentGenerator->OnRowsRefreshed().AddSP(this, &SDevKitSceneWorkbenchWidget::QueueEnvironmentRefresh);
    EnvironmentGenerator->OnFinishedChangingProperties().AddLambda([this](const FPropertyChangedEvent& Event)
    {
        bEnvironmentDraftDirty = true;
        if (Event.GetPropertyName() == TEXT("Profile") || Event.GetPropertyName() == TEXT("bOverrideProfile")) QueueEnvironmentRefresh();
    });
    EnvironmentGenerator->SetObjects({EnvironmentDraft.Get()});
    EnvironmentValuesGenerator = PropertyModule.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
    EnvironmentValuesGenerator->OnFinishedChangingProperties().AddLambda([this](const FPropertyChangedEvent&)
    {
        if (!CanEditEnvironmentValues()) return;
        EnvironmentDraft->Settings = EnvironmentValues->Settings;
        bEnvironmentDraftDirty = true;
    });
    EnvironmentValues->Settings = GetEnvironmentDraftSettings();
    EnvironmentValuesGenerator->SetObjects({EnvironmentValues.Get()});
}

FYogLevelEnvironmentSettings SDevKitSceneWorkbenchWidget::GetEnvironmentDraftSettings() const
{
    return EnvironmentDraft.IsValid() ? EnvironmentDraft->GetEffectiveSettings() : GetYogGlobalEnvironmentSettings();
}

bool SDevKitSceneWorkbenchWidget::CanEditEnvironmentValues() const
{
    return false;
}

void SDevKitSceneWorkbenchWidget::SetEnvironmentOverride(bool bOverride)
{
    // Old callers cannot reactivate retired rendering overrides.
    Status = FText::FromString(TEXT("已恢复 UE 原生光照；不再创建或应用自定义分层 / 曝光覆盖。"));
}

void SDevKitSceneWorkbenchWidget::RestoreGlobalEnvironment()
{
    // Retain dormant local/profile values, rather than resetting or deleting them.
    SetEnvironmentOverride(false);
}

void SDevKitSceneWorkbenchWidget::SetNativeEnvironment()
{
    SetEnvironmentOverride(false);
}

FText SDevKitSceneWorkbenchWidget::GetEnvironmentSourceText() const
{
    FString Result = TEXT("当前使用 UE 原生光照；旧关卡环境中的分层与曝光参数不再参与渲染。");
    if (EnvironmentEditedActor.IsValid())
        Result += FString::Printf(TEXT("\n备忘存储：%s（只保存备注，不应用旧渲染设置）"), *EnvironmentEditedActor->GetActorLabel());
    if (bEnvironmentDraftDirty) Result += TEXT("\n环境备忘尚未写入关卡。");
    return FText::FromString(Result);
}

void SDevKitSceneWorkbenchWidget::QueueEnvironmentRefresh()
{
    if (bEnvironmentRefreshQueued) return;
    bEnvironmentRefreshQueued = true;
    const TWeakPtr<SDevKitSceneWorkbenchWidget> WeakThis = StaticCastSharedRef<SDevKitSceneWorkbenchWidget>(AsShared());
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
    {
        if (auto Widget = WeakThis.Pin()) {Widget->bEnvironmentRefreshQueued = false; if (Widget->ActivePage == 2) Widget->RebuildRightPanel();}
        return false;
    }));
}

bool SDevKitSceneWorkbenchWidget::ConfirmDiscardEnvironmentDraft()
{
    if (!bEnvironmentDraftDirty && !bNativeStyleDraftDirty) return true;
    if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(TEXT("环境备注或原生调色覆盖尚未写入关卡。继续将丢弃草稿；选择否可返回 Apply。"))) != EAppReturnType::Yes) return false;
    bEnvironmentDraftDirty = false; bNativeStyleDraftDirty = false; return true;
}

TSharedRef<SWidget> SDevKitSceneWorkbenchWidget::MakeEnvironmentPanel()
{
    if (!GetSelectedEditorWorld() || !EnvironmentDraft.IsValid()) return Card(TEXT("美术环境"), TEXT("使用 UE 原生灯光、Lumen 与相机 / Post Process 曝光。打开所选关卡可编辑环境备忘；灯光子关卡仍在子关卡资源中管理。"), Button(TEXT("打开所选关卡"), [this](){return OpenSelectedMap();}, true));
    TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
    Body->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[MakeNativeStylePanel()];
    Body->AddSlot().AutoHeight().Padding(0, 0, 0, 10)
        [SNew(STextBlock).Text(this, &SDevKitSceneWorkbenchWidget::GetEnvironmentSourceText)
            .ColorAndOpacity(Green).AutoWrapText(true)];
    if (const TSharedPtr<IPropertyHandle> Notes = FindProperty(EnvironmentGenerator->GetRootTreeNodes(), TEXT("Notes")))
    {
        Body->AddSlot().AutoHeight()[Field(TEXT("环境备忘"), Notes->CreatePropertyValueWidget())];
    }
    Body->AddSlot().AutoHeight().Padding(0, 10)[Text(TEXT("已移除自定义 Lumen 分层、颜色分层后处理和额外曝光补偿。场景光源、SkyLight、Post Process 仍在关卡中按 UE 原生方式编辑；本页不会修改这些对象。"), FSlateColor::UseSubduedForeground())];
    Body->AddSlot().AutoHeight()[SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8, 6))
        + SWrapBox::Slot()[Button(TEXT("写入环境备忘"), [this](){return ApplyEnvironment();}, true)]
        + SWrapBox::Slot()[Button(TEXT("灯光 / 子关卡资源"), [this](){ActivePage = 3; RebuildRightPanel(); return FReply::Handled();})]
        + SWrapBox::Slot()[Button(TEXT("重读关卡"), [this](){if (ConfirmDiscardEnvironmentDraft()) {LoadEnvironmentDraft(); RebuildRightPanel();} return FReply::Handled();})]];
    return Card(TEXT("美术环境 / 原生调色、灯光与备忘"), TEXT("默认继承全局风格。关卡可显式覆盖原生对比度和饱和度；不改变场景 Shader、Lumen GI、VSM 或曝光。Apply 后仍需保存主关卡。"), Body);
}

TSharedRef<SWidget> SDevKitSceneWorkbenchWidget::MakeNativeStylePanel()
{
    TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
    Body->AddSlot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]()
    {
        FString Source; ResolveYogNativeRenderStyle(GetSelectedEditorWorld(), &Source);
        if (bNativeStyleDraftDirty) Source += TEXT("\n草稿尚未 Apply；当前画面仍使用已应用配置。");
        return FText::FromString(Source);
    }).ColorAndOpacity(Blue).AutoWrapText(true)];
    Body->AddSlot().AutoHeight().Padding(0, 8)[SNew(SCheckBox)
        .IsChecked_Lambda([this]() {return bOverrideNativeStyleDraft ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
        .OnCheckStateChanged_Lambda([this](ECheckBoxState State) {bOverrideNativeStyleDraft = State == ECheckBoxState::Checked; bNativeStyleDraftDirty = true;})
        [Text(TEXT("使用本关卡的原生调色覆盖（取消则继承全局）"))]];
    Body->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[SNew(SCheckBox)
        .IsEnabled_Lambda([this]() {return bOverrideNativeStyleDraft;})
        .IsChecked_Lambda([this]() {return NativeStyleDraft.bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
        .OnCheckStateChanged_Lambda([this](ECheckBoxState State) {NativeStyleDraft.bEnabled = State == ECheckBoxState::Checked; bNativeStyleDraftDirty = true;})
        [Text(TEXT("启用本关卡调色（关闭保持原生；仍受全局总开关控制）"))]];
    auto AddValue = [this, Body](const FString& Label, float FYogNativeRenderStyleSettings::* Member, float Min, float Max)
    {
        Body->AddSlot().AutoHeight()[Field(Label, SNew(SNumericEntryBox<float>)
            .IsEnabled_Lambda([this]() {return bOverrideNativeStyleDraft;})
            .MinValue(Min).MaxValue(Max).MinSliderValue(Min).MaxSliderValue(Max)
            .Value_Lambda([this, Member]() -> TOptional<float>
            {return bOverrideNativeStyleDraft ? NativeStyleDraft.*Member : GetDefault<UYogRenderStyleSettings>()->Style.*Member;})
            .OnValueChanged_Lambda([this, Member](float Value) {NativeStyleDraft.*Member = Value; bNativeStyleDraftDirty = true;}))];
    };
    AddValue(TEXT("全局对比度"), &FYogNativeRenderStyleSettings::GlobalContrast, .5f, 1.5f);
    AddValue(TEXT("饱和度"), &FYogNativeRenderStyleSettings::GlobalSaturation, 0.f, 2.f);
    AddValue(TEXT("暗部对比度"), &FYogNativeRenderStyleSettings::ShadowsContrast, .5f, 1.5f);
    AddValue(TEXT("明暗分区对比（中间调）"), &FYogNativeRenderStyleSettings::MidtonesContrast, .5f, 1.5f);
    AddValue(TEXT("暗部细节（Gamma）"), &FYogNativeRenderStyleSettings::ShadowsGamma, .8f, 1.2f);
    Body->AddSlot().AutoHeight()[Field(TEXT("覆盖优先级"), SNew(SNumericEntryBox<int32>)
        .IsEnabled_Lambda([this]() {return bOverrideNativeStyleDraft;})
        .Value_Lambda([this]() -> TOptional<int32> {return NativeStylePriorityDraft;})
        .OnValueChanged_Lambda([this](int32 Value) {NativeStylePriorityDraft = Value; bNativeStyleDraftDirty = true;}))];
    Body->AddSlot().AutoHeight().Padding(0, 8)[Text(TEXT("仅叠加 UE 原生调色倍率，保留关卡 RGB 调色、电影曲线与原生曝光；没有额外全屏 Shader。总开关关闭时所有关卡退出全局风格。"), FSlateColor::UseSubduedForeground())];
    Body->AddSlot().AutoHeight()[SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(8, 6))
        + SWrapBox::Slot()[Button(TEXT("Apply · 写入关卡覆盖"), [this]() {return ApplyNativeStyle();}, true)]
        + SWrapBox::Slot()[Button(TEXT("Default · 原生调色草稿"), [this]() {NativeStyleDraft = FYogNativeRenderStyleSettings(); bOverrideNativeStyleDraft = true; bNativeStyleDraftDirty = true; return FReply::Handled();})]
        + SWrapBox::Slot()[Button(TEXT("继承全局"), [this]() {bOverrideNativeStyleDraft = false; bNativeStyleDraftDirty = true; return FReply::Handled();})]];
    return Body;
}

FReply SDevKitSceneWorkbenchWidget::ApplyNativeStyle()
{
    UWorld* World = GetSelectedEditorWorld();
    if (!World || !GEditor || GEditor->IsPlaySessionInProgress()) return FReply::Handled();
    AYogRenderStyleActor* Actor = NativeStyleActor.Get();
    if (Actor && Actor->GetLevel() != World->PersistentLevel) Actor = nullptr;
    if (!Actor && !bOverrideNativeStyleDraft)
    {
        bNativeStyleDraftDirty = false;
        Status = FText::FromString(TEXT("当前关卡继承全局风格，未创建 Actor 或修改地图。"));
        return FReply::Handled();
    }
    const FScopedTransaction Transaction(FText::FromString(TEXT("应用关卡原生调色覆盖")));
    if (!Actor)
    {
        FActorSpawnParameters Params; Params.OverrideLevel = World->PersistentLevel; Params.ObjectFlags |= RF_Transactional;
        Actor = World->SpawnActor<AYogRenderStyleActor>(Params);
        if (Actor) Actor->SetActorLabel(TEXT("Native Render Style / 原生调色覆盖"));
    }
    if (!Actor || !Actor->StyleComponent) return FReply::Handled();
    Actor->Modify(); Actor->StyleComponent->Modify();
    Actor->StyleComponent->bOverrideGlobalStyle = bOverrideNativeStyleDraft;
    Actor->StyleComponent->Priority = NativeStylePriorityDraft;
    Actor->StyleComponent->Style = NativeStyleDraft.Sanitized();
    Actor->MarkPackageDirty(); NativeStyleActor = Actor;
    bNativeStyleDraftDirty = false;
    GEditor->RedrawLevelEditingViewports();
    Status = FText::FromString(TEXT("已 Apply 原生调色覆盖（可撤销）；点击保存所选配置才写入地图。"));
    return FReply::Handled();
}

FReply SDevKitSceneWorkbenchWidget::ApplyEnvironment()
{
    UWorld* World = GetSelectedEditorWorld();
    if (!World || !EnvironmentDraft.IsValid() || GEditor->IsPlaySessionInProgress()) return FReply::Handled();
    AYogLevelEnvironment* Actor = EnvironmentEditedActor.IsValid() && EnvironmentEditedActor->GetLevel() == World->PersistentLevel
        ? EnvironmentEditedActor.Get() : FindEditablePersistentEnvironment(World);
    if (!Actor && EnvironmentDraft->Notes.IsEmpty())
    {
        bEnvironmentDraftDirty = false;
        Status = FText::FromString(TEXT("环境备忘为空；未创建 Actor、未改动或标记地图。"));
        return FReply::Handled();
    }
    const FScopedTransaction Transaction(FText::FromString(TEXT("写入关卡环境备忘")));
    if (!Actor)
    {
        FActorSpawnParameters Params; Params.OverrideLevel = World->PersistentLevel; Params.ObjectFlags |= RF_Transactional;
        Actor = World->SpawnActor<AYogLevelEnvironment>(Params);
        if (Actor) Actor->SetActorLabel(TEXT("Level Environment Notes / 环境备忘"));
    }
    if (!Actor || !Actor->EnvironmentComponent) return FReply::Handled();
    Actor->Modify(); UYogLevelEnvironmentComponent* Target = Actor->EnvironmentComponent; Target->Modify();
    // Preserve legacy serialized rendering data; only the non-rendering note is edited.
    Target->Notes = EnvironmentDraft->Notes;
    Actor->MarkPackageDirty();
    EnvironmentEditedActor = Actor;
    bEnvironmentDraftDirty = false;
    Status = FText::FromString(TEXT("环境备忘已写入（可撤销）；未修改任何渲染参数。点击保存所选配置写入地图。"));
    return FReply::Handled();
}

FReply SDevKitSceneWorkbenchWidget::SaveEnvironmentPreset()
{
    Status = FText::FromString(TEXT("自定义分层 / 曝光预设已移除；未创建或保存任何资产。"));
    return FReply::Handled();
}
