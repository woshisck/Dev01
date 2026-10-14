#include "Tools/CharacterLookDev/SDevKitCharacterLookDevWidget.h"

#include "Components/LightComponent.h"
#include "DetailsViewArgs.h"
#include "Editor.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "IDetailsView.h"
#include "LookDev/YogLookDevLightingRig.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SDevKitCharacterLookDevWidget"

namespace
{
TSharedRef<SWidget> MakeSection(const FText& Title, const TSharedRef<SWidget>& Content)
{
	return SNew(SExpandableArea)
		.InitiallyCollapsed(false)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.HeaderPadding(FMargin(8.f, 6.f))
		.Padding(FMargin(8.f, 6.f))
		.HeaderContent()
		[
			SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle("BoldFont"))
		]
		.BodyContent()[Content];
}

bool IsVisibleProperty(const FPropertyAndParent& Property, const TArray<FName>& AllowedRoots)
{
	if (AllowedRoots.Contains(Property.Property.GetFName())) return true;
	for (const FProperty* Parent : Property.ParentProperties)
	{
		if (Parent && AllowedRoots.Contains(Parent->GetFName())) return true;
	}
	return false;
}

void SetPropertyFilter(const TSharedPtr<IDetailsView>& View, TArray<FName> Roots)
{
	View->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda(
		[Roots = MoveTemp(Roots)](const FPropertyAndParent& Property)
		{
			return IsVisibleProperty(Property, Roots);
		}));
}

TSharedRef<IDetailsView> MakeDetails(FName Identifier, bool bSearch)
{
	FDetailsViewArgs Args;
	Args.ViewIdentifier = Identifier;
	Args.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	Args.bUpdatesFromSelection = false;
	Args.bLockable = false;
	Args.bAllowSearch = bSearch;
	Args.bHideSelectionTip = true;
	Args.bShowObjectLabel = false;
	Args.bShowOptions = false;
	Args.bShowSectionSelector = false;
	Args.bShowPropertyMatrixButton = false;
	Args.bShowScrollBar = false;
	Args.bShowLooseProperties = false;
	Args.ColumnWidth = .50f;
	Args.RightColumnMinWidth = 80.f;
	TSharedRef<IDetailsView> View = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(Args);
	// Actor/component customizations add unrelated transform and CallInEditor rows;
	// this panel deliberately exposes only its property whitelist.
	View->SetDisableCustomDetailLayouts(true);
	View->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateLambda([](const FPropertyAndParent& Property)
	{
		// Display the authored native unit, but never convert units through this comparison panel.
		return Property.Property.GetFName() == TEXT("IntensityUnits");
	}));
	return View;
}
}

void SDevKitCharacterLookDevWidget::Construct(const FArguments& InArgs)
{
	EnvironmentDetails = MakeDetails(TEXT("YogLookDev.Environment"), false);
	DirectionalDetails = MakeDetails(TEXT("YogLookDev.Directional"), true);
	SkyDetails = MakeDetails(TEXT("YogLookDev.Sky"), true);
	SetPropertyFilter(EnvironmentDetails, { TEXT("LightingScenario"), TEXT("BackgroundCompensation"),
		TEXT("ExposureCompensation"), TEXT("bGrounded"), TEXT("bCalibrator") });
	for (const TSharedPtr<IDetailsView>& View : { EnvironmentDetails, DirectionalDetails, SkyDetails })
	{
		View->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateSP(this,
			&SDevKitCharacterLookDevWidget::CanEditLighting));
	}
	RefreshEntries();
	StatusText = LOCTEXT("InitialStatus", "选择 Review 后加载；切换灯光只改变灯光子关卡，保留同一组评审资产。");
	ChildSlot
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(10.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
				[SNew(STextBlock).Text(LOCTEXT("Title", "LookDev / 光照对比")).Font(FAppStyle::GetFontStyle("BoldFont"))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
				[SNew(STextBlock).Text(LOCTEXT("Intro", "同一资产 · 不同照明。参数保存在灯光关卡，不修改项目全局渲染配置。")).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeSection(LOCTEXT("LevelSection", "关卡"),
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SAssignNew(ReviewCombo, SComboBox<FReviewOption>)
								.OptionsSource(&ReviewOptions).InitiallySelectedItem(SelectedReview)
								.OnSelectionChanged(this, &SDevKitCharacterLookDevWidget::OnReviewSelected)
								.OnGenerateWidget_Lambda([](FReviewOption Option)
								{
									return SNew(STextBlock).Text(Option.IsValid() ? Option->Label : FText::GetEmpty());
								})
								[SNew(STextBlock).Text(this, &SDevKitCharacterLookDevWidget::GetSelectedReviewText)]
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
							[SNew(SButton).Text(LOCTEXT("LoadLevel", "加载关卡")).IsEnabled(this, &SDevKitCharacterLookDevWidget::CanLoadReview).OnClicked(this, &SDevKitCharacterLookDevWidget::LoadSelectedReview)]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
						[SNew(STextBlock).Text(this, &SDevKitCharacterLookDevWidget::GetCurrentLevelText).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())])
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
				[
					MakeSection(LOCTEXT("LightSection", "灯光关卡"), SAssignNew(LightingButtons, SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6.f, 6.f)))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[SNew(STextBlock).Text_Lambda([this] { return LightingTargetHint; }).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
					+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
					[SNew(SButton).Text(LOCTEXT("RefreshLighting", "刷新灯光")).OnClicked(this, &SDevKitCharacterLookDevWidget::RefreshPanel)]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(4.f)
					.Visibility_Lambda([this] { return BoundRig.IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[EnvironmentDetails.ToSharedRef()]
						+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
						[SNew(STextBlock).Text(LOCTEXT("ScenarioHint", "Lighting Scenarios 切换 Low / Mid / High 时恢复该方案的灯光强度；随后可独立微调下方数据。")).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SSplitter).Orientation(Orient_Horizontal)
					+ SSplitter::Slot().Value(.5f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 8.f)
						[SNew(STextBlock).Text(this, &SDevKitCharacterLookDevWidget::GetDirectionalHeading).AutoWrapText(true).Font(FAppStyle::GetFontStyle("BoldFont"))]
						+ SVerticalBox::Slot().AutoHeight()[DirectionalDetails.ToSharedRef()]
					]
					+ SSplitter::Slot().Value(.5f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 8.f)
						[SNew(STextBlock).Text(this, &SDevKitCharacterLookDevWidget::GetSkyHeading).AutoWrapText(true).Font(FAppStyle::GetFontStyle("BoldFont"))]
						+ SVerticalBox::Slot().AutoHeight()[SkyDetails.ToSharedRef()]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(4.f, 10.f)
				[
					SNew(STextBlock).AutoWrapText(true)
					.Text_Lambda([this] { return StatusText; })
					.ColorAndOpacity_Lambda([this] { return bStatusIsError ? FSlateColor(FLinearColor(1.f, .35f, .25f)) : FSlateColor::UseForeground(); })
				]
			]
		]
	];
	RebuildLightingButtons();
	RefreshLightingTargets(true);
}

void SDevKitCharacterLookDevWidget::Activate()
{
	FText Message;
	const bool bSuccess = FDevKitLookDevLevelService::OpenMain(Message);
	SetStatus(bSuccess, Message);
	RefreshEntries();
	RefreshLightingTargets(true);
	UE_LOG(LogTemp, Display, TEXT("LOOKDEV_WORKBENCH_OPEN success=%s review=%s light=%s message=%s"),
		bSuccess ? TEXT("true") : TEXT("false"), *FDevKitLookDevLevelService::GetCurrentReviewPath(),
		*FDevKitLookDevLevelService::GetCurrentLightingPath(), *Message.ToString());
}

void SDevKitCharacterLookDevWidget::Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (InCurrentTime >= NextRefreshTime)
	{
		NextRefreshTime = InCurrentTime + .5;
		// Rebind after map switches, Blueprint reconstruction or undo, without writing scene state.
		RefreshLightingTargets();
	}
}

void SDevKitCharacterLookDevWidget::RefreshEntries()
{
	const FString Current = FDevKitLookDevLevelService::GetCurrentReviewPath();
	const FString Previous = SelectedReview.IsValid() ? SelectedReview->PackagePath : FString();
	ReviewOptions.Reset();
	SelectedReview.Reset();
	for (const FDevKitLookDevLevelEntry& Entry : FDevKitLookDevLevelService::GetReviewEntries())
	{
		if (Entry.bStandalone) continue;
		FReviewOption Option = MakeShared<FDevKitLookDevLevelEntry>(Entry);
		ReviewOptions.Add(Option);
		if (Entry.PackagePath == (Current.IsEmpty() ? Previous : Current)) SelectedReview = Option;
	}
	if (!SelectedReview.IsValid() && !ReviewOptions.IsEmpty()) SelectedReview = ReviewOptions[0];
	LightingEntries = FDevKitLookDevLevelService::GetLightingEntries();
	if (ReviewCombo.IsValid())
	{
		ReviewCombo->RefreshOptions();
		ReviewCombo->SetSelectedItem(SelectedReview);
	}
	RebuildLightingButtons();
}

void SDevKitCharacterLookDevWidget::RebuildLightingButtons()
{
	if (!LightingButtons.IsValid()) return;
	LightingButtons->ClearChildren();
	for (const FDevKitLookDevLevelEntry& Entry : LightingEntries)
	{
		const FString Path = Entry.PackagePath;
		const FText Label = Entry.Label;
		LightingButtons->AddSlot()
		[
			SNew(SButton).ContentPadding(FMargin(10.f, 7.f))
			.IsEnabled_Lambda([] { return GEditor && !GEditor->PlayWorld && FDevKitLookDevLevelService::IsMainWorld(); })
			.ToolTipText(Entry.Description)
			.OnClicked(this, &SDevKitCharacterLookDevWidget::SwitchLighting, Path)
			.ButtonColorAndOpacity_Lambda([Path]
			{
				return FDevKitLookDevLevelService::GetCurrentLightingPath() == Path ? FLinearColor(.10f, .32f, .52f) : FLinearColor::White;
			})
			[SNew(STextBlock).Text(Label)]
		];
	}
}

void SDevKitCharacterLookDevWidget::RefreshLightingTargets(bool bForce)
{
	if (!EnvironmentDetails.IsValid()) return;
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	const FString LightingPath = FDevKitLookDevLevelService::GetCurrentLightingPath();
	TArray<TWeakObjectPtr<UObject>> DirectionalObjects;
	TArray<TWeakObjectPtr<UObject>> SkyObjects;
	AYogLookDevLightingRig* Rig = nullptr;
	bool bComponentsEditable = true;
	int32 RigCount = 0;
	if (World && FDevKitLookDevLevelService::IsMainWorld() && !LightingPath.IsEmpty())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			// Never expose another loaded/hidden light scene or the review asset's own components.
			if (!Actor || !Actor->GetLevel() || !Actor->GetLevel()->bIsVisible
				|| Actor->GetLevel()->GetOutermost()->GetName() != LightingPath) continue;
			if (AYogLookDevLightingRig* Candidate = Cast<AYogLookDevLightingRig>(Actor))
			{
				++RigCount;
				if (!Rig) Rig = Candidate;
				continue;
			}
			// The saved L1/L2/L3 builders identify roles with these authored labels.
			// Do not merge unrelated Point/Rect lights or assume their intensity units match.
			const FString Label = Actor->GetActorLabel();
			const bool bKey = Label.Contains(TEXT("LookDev Candidate Key"));
			const bool bFill = Label.Contains(TEXT("LookDev Candidate Broad Fill"));
			if (bKey == bFill) continue;
			TInlineComponentArray<ULightComponent*> Lights;
			Actor->GetComponents(Lights);
			for (ULightComponent* Light : Lights)
			{
				if (Light && Light->IsRegistered() && Light->IsVisible())
				{
					(bKey ? DirectionalObjects : SkyObjects).Add(Light);
					bComponentsEditable &= Actor->GetClass()->ClassGeneratedBy == nullptr;
				}
			}
		}
	}
	bool bRigComplete = false;
	bool bHasDirectional = false;
	bool bHasSky = false;
	if (RigCount > 1)
	{
		// Do not select an arbitrary rig while multiple rigs contribute illumination.
		Rig = nullptr;
		DirectionalObjects.Reset();
		SkyObjects.Reset();
		bComponentsEditable = false;
	}
	else if (Rig)
	{
		bHasDirectional = Rig->GetActiveDirectionalLight() != nullptr;
		bHasSky = Rig->GetSkyLight() != nullptr;
		bRigComplete = bHasDirectional && bHasSky;
		DirectionalObjects.Reset();
		SkyObjects.Reset();
		if (bHasDirectional) DirectionalObjects.Add(Rig);
		if (bHasSky) SkyObjects.Add(Rig);
		// Keep the surviving data visible, but prevent edits to an invalid lighting rig.
		bComponentsEditable = bRigComplete;
	}
	const bool bAmbiguousRoles = !Rig && (DirectionalObjects.Num() > 1 || SkyObjects.Num() > 1);
	if (bAmbiguousRoles)
	{
		// A duplicated label is not permission to multi-edit lights. The artist can disambiguate in the level.
		if (DirectionalObjects.Num() > 1) DirectionalObjects.Reset();
		if (SkyObjects.Num() > 1) SkyObjects.Reset();
		bComponentsEditable = false;
	}
	const bool bUnchanged = BoundRig.Get() == Rig && BoundDirectionalObjects == DirectionalObjects
		&& BoundSkyObjects == SkyObjects && BoundLightingPath == LightingPath
		&& bNativeComponentsEditable == bComponentsEditable && BoundRigCount == RigCount
		&& bBoundRigComplete == bRigComplete;
	if (!bForce && bUnchanged) return;
	BoundRig = Rig;
	BoundRigCount = RigCount;
	bBoundRigComplete = bRigComplete;
	BoundLightingPath = LightingPath;
	BoundDirectionalObjects = MoveTemp(DirectionalObjects);
	BoundSkyObjects = MoveTemp(SkyObjects);
	bNativeComponentsEditable = bComponentsEditable;
	SetPropertyFilter(DirectionalDetails, Rig ? TArray<FName>{ TEXT("DirectionalSettings") }
		: TArray<FName>{ TEXT("Intensity"), TEXT("IntensityUnits"), TEXT("LightColor"), TEXT("IndirectLightingIntensity"), TEXT("VolumetricScatteringIntensity") });
	SetPropertyFilter(SkyDetails, Rig ? TArray<FName>{ TEXT("SkySettings") }
		: TArray<FName>{ TEXT("Intensity"), TEXT("IntensityUnits"), TEXT("LightColor"), TEXT("IndirectLightingIntensity"), TEXT("VolumetricScatteringIntensity") });
	EnvironmentDetails->SetObject(Rig, true);
	DirectionalDetails->SetObjects(BoundDirectionalObjects, true);
	SkyDetails->SetObjects(BoundSkyObjects, true);
	EnvironmentDetails->SetRootExpansionStates(true, true);
	DirectionalDetails->SetRootExpansionStates(true, true);
	SkyDetails->SetRootExpansionStates(true, true);
	if (RigCount > 1)
	{
		LightingTargetHint = FText::Format(LOCTEXT("MultipleRigHint", "当前有 {0} 个灯光 Rig；为避免编辑错误对象，已清空参数绑定。请在关卡中保留唯一 Rig 后重试。"), FText::AsNumber(RigCount));
	}
	else if (Rig && !bRigComplete)
	{
		const FText Missing = !bHasDirectional && !bHasSky
			? LOCTEXT("MissingBothLights", "当前方案 DirectionalLight 与 Skylight")
			: (!bHasDirectional ? LOCTEXT("MissingDirectional", "当前方案 DirectionalLight") : LOCTEXT("MissingSky", "Skylight"));
		LightingTargetHint = FText::Format(LOCTEXT("IncompleteRigHint", "Rig 缺少 {0} 组件，已清空对应参数列并禁用编辑。请修复 AssetsPreview_BP 的组件或完成迁移；仅放置原生 Rig 不会自动创建灯光。"), Missing);
	}
	else if (Rig)
	{
		LightingTargetHint = LOCTEXT("RigHint", "AssetsPreview · 参数持久保存在当前灯光关卡。支持撤销；修改后请保存该关卡。");
	}
	else if (bAmbiguousRoles)
	{
		LightingTargetHint = LOCTEXT("AmbiguousLightRoles", "发现重复的 Key / Broad Fill 灯光角色；未执行多灯合并编辑。请在关卡中确认并区分这些灯光后刷新。");
	}
	else if (!BoundDirectionalObjects.IsEmpty() || !BoundSkyObjects.IsEmpty())
	{
		LightingTargetHint = bComponentsEditable
			? LOCTEXT("NativeHint", "此方案使用原生 Key / Broad Fill 灯光；每列只编辑一个灯光，保留其原生强度单位。缺失角色的列保持空白；修改后请保存灯光关卡。")
			: LOCTEXT("LegacyBlueprintHint", "检测到尚未迁移的蓝图灯光；只读显示，避免构造脚本覆盖参数。请完成 AssetsPreview 的 Rig 迁移后编辑。");
	}
	else
	{
		LightingTargetHint = LightingPath.IsEmpty()
			? LOCTEXT("NoLightingHint", "先加载 LookDev Review，再选择灯光关卡。此处仅显示当前灯光关卡，不会修改其他世界的灯光。")
			: LOCTEXT("UnidentifiedLights", "当前灯光关卡未找到可用 Rig 或唯一的 Candidate Key / Broad Fill 灯光。请在关卡中选择并检查灯光；面板不会任意合并其他组件。");
	}
}

void SDevKitCharacterLookDevWidget::SetStatus(bool bSuccess, const FText& Message)
{
	bStatusIsError = !bSuccess;
	StatusText = Message;
}

void SDevKitCharacterLookDevWidget::OnReviewSelected(FReviewOption Option, ESelectInfo::Type SelectInfo)
{
	SelectedReview = Option;
}

FReply SDevKitCharacterLookDevWidget::LoadSelectedReview()
{
	if (SelectedReview.IsValid())
	{
		FText Message;
		const bool bSuccess = FDevKitLookDevLevelService::LoadReview(SelectedReview->PackagePath, Message);
		SetStatus(bSuccess, Message);
		RefreshLightingTargets(true);
	}
	return FReply::Handled();
}

FReply SDevKitCharacterLookDevWidget::SwitchLighting(FString PackagePath)
{
	FText Message;
	const bool bSuccess = FDevKitLookDevLevelService::SwitchLighting(PackagePath, Message);
	SetStatus(bSuccess, Message);
	RefreshLightingTargets(true);
	return FReply::Handled();
}

FReply SDevKitCharacterLookDevWidget::RefreshPanel()
{
	RefreshEntries();
	RefreshLightingTargets(true);
	SetStatus(true, LOCTEXT("RefreshSuccess", "已重新读取当前灯光关卡；未修改或保存任何参数。"));
	return FReply::Handled();
}

bool SDevKitCharacterLookDevWidget::CanLoadReview() const
{
	return GEditor && !GEditor->PlayWorld && SelectedReview.IsValid();
}

bool SDevKitCharacterLookDevWidget::CanEditLighting() const
{
	return GEditor && !GEditor->PlayWorld && FDevKitLookDevLevelService::IsMainWorld() && bNativeComponentsEditable;
}

FText SDevKitCharacterLookDevWidget::GetSelectedReviewText() const
{
	return SelectedReview.IsValid() ? SelectedReview->Label : LOCTEXT("NoReview", "没有可用的 Review 关卡");
}

FText SDevKitCharacterLookDevWidget::GetCurrentLevelText() const
{
	const FString Current = FDevKitLookDevLevelService::GetCurrentReviewPath();
	for (const FReviewOption& Option : ReviewOptions)
	{
		if (Option->PackagePath == Current) return FText::Format(LOCTEXT("CurrentReview", "当前：{0}"), Option->Label);
	}
	return LOCTEXT("NoReviewLoaded", "当前未加载 Review；加载操作将进入 YogLookDev 插件关卡。");
}

FText SDevKitCharacterLookDevWidget::GetDirectionalHeading() const
{
	if (!BoundRig.IsValid() && BoundDirectionalObjects.Num() == 1)
	{
		const ULightComponent* Light = Cast<ULightComponent>(BoundDirectionalObjects[0].Get());
		if (Light && Light->GetOwner()) return FText::Format(LOCTEXT("NativeKeyHeading", "Key Light / 主光\n{0}"), FText::FromString(Light->GetOwner()->GetActorLabel()));
	}
	return LOCTEXT("DirectionalHeading", "Directional Light / 主光");
}

FText SDevKitCharacterLookDevWidget::GetSkyHeading() const
{
	if (!BoundRig.IsValid() && BoundSkyObjects.Num() == 1)
	{
		const ULightComponent* Light = Cast<ULightComponent>(BoundSkyObjects[0].Get());
		if (Light && Light->GetOwner()) return FText::Format(LOCTEXT("NativeFillHeading", "Fill Light / 补光\n{0}"), FText::FromString(Light->GetOwner()->GetActorLabel()));
	}
	return LOCTEXT("SkyHeading", "Sky Light / 环境光");
}

#undef LOCTEXT_NAMESPACE
