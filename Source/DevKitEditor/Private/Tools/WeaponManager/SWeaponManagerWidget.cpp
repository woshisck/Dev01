#include "Tools/WeaponManager/SWeaponManagerWidget.h"

#include "AbilitySystem/Abilities/GA_WeaponSkill.h"
#include "Animation/AnimMontage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetThumbnail.h"
#include "AssetToolsModule.h"
#include "Data/AbilityData.h"
#include "Data/WeaponSkillDataAsset.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "IDetailsView.h"
#include "InputCoreTypes.h"
#include "IAssetTools.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInfoDA.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Tools/WeaponManager/WeaponDefinitionActorFactory.h"

#define LOCTEXT_NAMESPACE "WeaponManager"

namespace
{
	const FString OfficialWeaponRoot(TEXT("/Game/Code/Weapon"));

	bool IsOfficialWeaponPackage(const FString& PackageName)
	{
		return PackageName == OfficialWeaponRoot
			|| PackageName.StartsWith(OfficialWeaponRoot + TEXT("/"));
	}

	FText GetWeaponSkillMontageSlotLabel(
		const FGameplayTag& SkillTag,
		const FGameplayTag& MontageSlot)
	{
		const FString SkillName = SkillTag.ToString();
		if (SkillName == TEXT("Weapon.Skill.Block"))
		{
			return LOCTEXT("BlockMontageSlot", "格挡动作");
		}
		if (SkillName == TEXT("Weapon.Skill.Thrust"))
		{
			return LOCTEXT("ThrustMontageSlot", "突刺动作");
		}

		const FString SlotName = MontageSlot.ToString();
		const int32 ComboMarker = SlotName.Find(TEXT("Combo"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (ComboMarker != INDEX_NONE)
		{
			const int32 ComboIndex = FCString::Atoi(*SlotName.Mid(ComboMarker + 5));
			if (ComboIndex > 0)
			{
				return FText::Format(
					LOCTEXT("WeaponSkillComboSlotFmt", "战技连段 {0}"),
					FText::AsNumber(ComboIndex));
			}
		}
		return FText::FromString(SlotName);
	}

	struct FWeaponActionRow
	{
		const TCHAR* Label;
		const TCHAR* Tag;
		const TCHAR* Description;
		bool bRuntimeReady;
	};

	const FWeaponActionRow BasicAttackRows[] = {
		{TEXT("普通攻击 1"), TEXT("Character.State.Skill.Attack.Combo1"), TEXT("普通攻击连段的第一段蒙太奇。"), true},
		{TEXT("普通攻击 2"), TEXT("Character.State.Skill.Attack.Combo2"), TEXT("普通攻击连段的第二段蒙太奇。"), true},
		{TEXT("普通攻击 3"), TEXT("Character.State.Skill.Attack.Combo3"), TEXT("普通攻击连段的第三段蒙太奇。"), true},
		{TEXT("普通攻击 4"), TEXT("Character.State.Skill.Attack.Combo4"), TEXT("普通攻击连段的第四段蒙太奇。"), true},
	};

	const FWeaponActionRow DashRows[] = {
		{TEXT("冲刺 / 闪避"), TEXT("Character.State.Movement.Dash"), TEXT("基础冲刺或闪避动作蒙太奇。"), true},
		{TEXT("冲刺攻击 1"), TEXT("Character.State.Movement.Dash.Combo1"), TEXT("从冲刺状态进入的第一段攻击。"), true},
		{TEXT("冲刺攻击 2"), TEXT("Character.State.Movement.Dash.Combo2"), TEXT("冲刺攻击连段的第二个槽位。"), true},
		{TEXT("冲刺攻击 3"), TEXT("Character.State.Movement.Dash.Combo3"), TEXT("冲刺攻击连段的第三个槽位。"), true},
		{TEXT("冲刺攻击 4"), TEXT("Character.State.Movement.Dash.Combo4"), TEXT("冲刺攻击连段的第四个槽位。"), true},
	};

	const FWeaponActionRow UtilityRows[] = {
		{TEXT("装填"), TEXT("Character.State.Skill.Reload"), TEXT("远程武器的装填蒙太奇；近战武器通常可以留空。"), true},
		{TEXT("切换武器"), TEXT("Character.State.Equipment.SwitchWeapon"), TEXT("主副武器切换动作蒙太奇。"), true},
	};

	const FWeaponActionRow PassiveRows[] = {
		{TEXT("正面受击"), TEXT("Action.HitReact.Front"), TEXT("已接入运行时：受到来自正面的攻击时播放。"), true},
		{TEXT("背面受击"), TEXT("Action.HitReact.Back"), TEXT("已接入运行时：受到来自背后的攻击时播放。"), true},
		{TEXT("格挡受击"), TEXT("Action.HitReact.Blocked"), TEXT("已接入运行时：成功格挡来袭攻击时播放。"), true},
		{TEXT("被弹反"), TEXT("Action.HitReact.Parried"), TEXT("已接入运行时：自身攻击被弹反后播放。"), true},
		{TEXT("死亡"), TEXT("Action.Dead"), TEXT("已接入运行时：死亡蒙太奇以及对应的消散 GameplayCue 数据。"), true},
		{TEXT("左侧受击"), TEXT("Action.HitReact.Left"), TEXT("预留：左侧方向受击反应标签。"), false},
		{TEXT("右侧受击"), TEXT("Action.HitReact.Right"), TEXT("预留：右侧方向受击反应标签。"), false},
		{TEXT("重硬直"), TEXT("Action.HitReact.Heavy"), TEXT("预留：重击造成的大硬直反应标签。"), false},
		{TEXT("眩晕"), TEXT("Action.Stun"), TEXT("预留：眩晕反应标签。"), false},
		{TEXT("破防"), TEXT("Action.GuardBreak"), TEXT("预留：防御被击破时的反应标签。"), false},
		{TEXT("倒地"), TEXT("Action.Knockdown"), TEXT("预留：进入倒地状态的反应标签。"), false},
		{TEXT("起身"), TEXT("Action.GetUp"), TEXT("预留：从倒地状态恢复的动作标签。"), false},
		{TEXT("击飞"), TEXT("Action.Launch"), TEXT("预留：被击飞至空中的反应标签。"), false},
		{TEXT("重落地"), TEXT("Action.Landing.Hard"), TEXT("预留：重落地反应标签。"), false},
		{TEXT("被处决"), TEXT("Action.Execution.Victim"), TEXT("预留：作为处决受害者时使用的蒙太奇标签。"), false},
		{TEXT("被背刺"), TEXT("Action.Backstab.Victim"), TEXT("预留：作为背刺受害者时使用的蒙太奇标签。"), false},
	};

	FSlateColor PageButtonColor(bool bSelected)
	{
		return bSelected
			? FSlateColor(FLinearColor(0.08f, 0.38f, 0.62f, 1.0f))
			: FSlateColor(FLinearColor(0.16f, 0.16f, 0.16f, 1.0f));
	}

	TSharedRef<SWidget> MakeSectionTitle(const FText& Title, const FText& Description)
	{
		return SNew(SBorder)
			.Padding(FMargin(10.f, 8.f))
			.BorderBackgroundColor(FLinearColor(0.07f, 0.07f, 0.07f, 1.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
						.Text(Title)
						.Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
						.Text(Description)
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			];
	}

	FString NormalizeGameFolder(FString Folder)
	{
		Folder.TrimStartAndEndInline();
		Folder.ReplaceInline(TEXT("\\"), TEXT("/"));
		while (Folder.EndsWith(TEXT("/")))
		{
			Folder.LeftChopInline(1);
		}
		return Folder;
	}
}

class SWeaponManagerDragCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWeaponManagerDragCard) {}
		SLATE_ARGUMENT(TSharedPtr<FWeaponManagerWeaponItem>, Item)
		SLATE_ARGUMENT(TWeakPtr<SWeaponManagerWidget>, OwnerWidget)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Item = InArgs._Item;
		OwnerWidget = InArgs._OwnerWidget;

		const bool bOfficial = Item.IsValid()
			&& Item->LibraryCategory == EWeaponManagerLibraryCategory::Official;
		const FText CategoryText = bOfficial
			? LOCTEXT("OfficialWeaponBadge", "正式")
			: LOCTEXT("TestWeaponBadge", "测试");
		const FLinearColor CategoryColor = bOfficial
			? FLinearColor(0.12f, 0.48f, 0.26f, 1.f)
			: FLinearColor(0.62f, 0.34f, 0.08f, 1.f);

		ChildSlot
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(56.f).HeightOverride(56.f)
				[
					Item.IsValid() && Item->Thumbnail.IsValid()
						? Item->Thumbnail->MakeThumbnailWidget()
						: SNullWidget::NullWidget
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(STextBlock)
							.Text(Item.IsValid() ? Item->DisplayName : FText::GetEmpty())
							.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(5.f, 0.f, 0.f, 0.f)
					[
						SNew(SBorder)
							.Padding(FMargin(4.f, 1.f))
							.BorderBackgroundColor(CategoryColor)
							[
								SNew(STextBlock)
									.Text(CategoryText)
									.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
							]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
						.Text(Item.IsValid() ? Item->WeaponTypeText : FText::GetEmpty())
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
						.Text(Item.IsValid() ? Item->ValidationText : FText::GetEmpty())
						.ColorAndOpacity(Item.IsValid() ? Item->ValidationColor : FSlateColor::UseForeground())
				]
			]
		];
	}

	virtual FReply OnMouseButtonDown(
		const FGeometry& MyGeometry,
		const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Item.IsValid())
		{
			if (const TSharedPtr<SWeaponManagerWidget> Owner = OwnerWidget.Pin())
			{
				Owner->SelectWeaponForInteraction(Item);
			}
			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
		}
		return FReply::Unhandled();
	}

	virtual FReply OnDragDetected(
		const FGeometry& MyGeometry,
		const FPointerEvent& MouseEvent) override
	{
		if (Item.IsValid())
		{
			if (const TSharedPtr<SWeaponManagerWidget> Owner = OwnerWidget.Pin())
			{
				return Owner->BeginWeaponDrag(Item);
			}
		}
		return FReply::Unhandled();
	}

private:
	TSharedPtr<FWeaponManagerWeaponItem> Item;
	TWeakPtr<SWeaponManagerWidget> OwnerWidget;
};

void SWeaponManagerWidget::Construct(const FArguments& InArgs)
{
	ThumbnailPool = MakeShared<FAssetThumbnailPool>(128);
	StatusText = LOCTEXT("InitialStatus", "武器管理器已就绪。");
	EnabledLibraryFilters.Add(EWeaponManagerLibraryCategory::Official);

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bLockable = false;
	DetailsArgs.bUpdatesFromSelection = false;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsArgs.NotifyHook = nullptr;

	FPropertyEditorModule& PropertyEditor =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	DetailsView = PropertyEditor.CreateDetailView(DetailsArgs);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			BuildToolbar()
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(3.f)
			+ SSplitter::Slot().Value(0.23f).MinSize(235.f)
			[
				BuildLeftPanel()
			]
			+ SSplitter::Slot().Value(0.47f).MinSize(430.f)
			[
				SAssignNew(CenterPanelBox, SBox)
			]
			+ SSplitter::Slot().Value(0.30f).MinSize(315.f)
			[
				BuildRightPanel()
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
				.Padding(FMargin(10.f, 6.f))
				.BorderBackgroundColor(FLinearColor(0.035f, 0.035f, 0.035f, 1.f))
				[
					SNew(STextBlock)
						.Text(this, &SWeaponManagerWidget::GetStatusText)
						.ColorAndOpacity(this, &SWeaponManagerWidget::GetStatusColor)
				]
		]
	];

	RefreshSkillTypes();
	RefreshWeapons(false);
	RebuildCenterPanel();
	RefreshDetailsPanel();
}

#if WITH_DEV_AUTOMATION_TESTS
int32 SWeaponManagerWidget::GetBasicActionRowCountForTesting() const
{
	return UE_ARRAY_COUNT(BasicAttackRows) + UE_ARRAY_COUNT(DashRows) + UE_ARRAY_COUNT(UtilityRows);
}

int32 SWeaponManagerWidget::GetPassiveActionRowCountForTesting() const
{
	return UE_ARRAY_COUNT(PassiveRows);
}

int32 SWeaponManagerWidget::GetNativeSkillTypeCountForTesting() const
{
	return SkillTypes.Num();
}

int32 SWeaponManagerWidget::GetRequiredMontageSlotCountForTesting(const FGameplayTag& SkillTag) const
{
	for (const FSkillTypePtr& SkillType : SkillTypes)
	{
		if (SkillType.IsValid() && SkillType->SkillTag.MatchesTagExact(SkillTag))
		{
			return SkillType->RequiredMontageSlots.Num();
		}
	}
	return INDEX_NONE;
}

bool SWeaponManagerWidget::HasCorePanelsForTesting() const
{
	return WeaponListView.IsValid()
		&& CenterPanelBox.IsValid()
		&& SkillRequirementsBox.IsValid()
		&& DetailsView.IsValid();
}

bool SWeaponManagerWidget::IsOfficialWeaponPathForTesting(const FString& PackageName)
{
	return IsOfficialWeaponPackage(PackageName);
}
#endif

TSharedRef<SWidget> SWeaponManagerWidget::BuildToolbar()
{
	return SNew(SBorder)
		.Padding(FMargin(10.f, 8.f))
		.BorderBackgroundColor(FLinearColor(0.025f, 0.025f, 0.025f, 1.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
					.Text(LOCTEXT("CreateWeapon", "新建武器"))
					.ToolTipText(LOCTEXT("CreateWeaponTip", "创建武器定义，以及配套的普通攻击、被动反应和显示信息 DA。"))
					.OnClicked(this, &SWeaponManagerWidget::OpenCreateWeaponDialog)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
					.Text(LOCTEXT("Refresh", "刷新"))
					.OnClicked_Lambda([this]()
					{
						RefreshSkillTypes();
						RefreshWeapons(true);
						RebuildCenterPanel();
						RefreshDetailsPanel();
						SetStatus(LOCTEXT("Refreshed", "已刷新武器资产和原生战技目录。"));
						return FReply::Handled();
					})
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton)
					.Text(LOCTEXT("SaveManaged", "签出并保存"))
					.ToolTipText(LOCTEXT("SaveManagedTip", "仅签出并保存当前选中的武器及本面板管理的配套 DA。"))
					.IsEnabled_Lambda([this]() { return GetSelectedWeapon() != nullptr; })
					.OnClicked(this, &SWeaponManagerWidget::SaveManagedAssets)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(14.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock)
					.Text(this, &SWeaponManagerWidget::GetSelectedWeaponTitle)
					.Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
					.Justification(ETextJustify::Right)
			]
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildLeftPanel()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 1.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT("WeaponLibrary", "武器库（可拖入关卡）"))
					.Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall")))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
					.Text(LOCTEXT(
						"WeaponLibraryPathRule",
						"正式武器目录：/Game/Code/Weapon；其他目录统一标记为“测试”。选中后按 Ctrl+B 可在内容浏览器中定位对应 DA。"))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SAssignNew(SearchBox, SSearchBox)
					.HintText(LOCTEXT("SearchWeapons", "搜索名称、路径或武器类型……"))
					.OnTextChanged(this, &SWeaponManagerWidget::HandleSearchChanged)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildLibraryFilterButtons()
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SAssignNew(WeaponListView, SListView<FWeaponItemPtr>)
					.ListItemsSource(&FilteredWeapons)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SWeaponManagerWidget::GenerateWeaponRow)
					.OnSelectionChanged(this, &SWeaponManagerWidget::HandleWeaponSelectionChanged)
					.OnKeyDownHandler(this, &SWeaponManagerWidget::HandleWeaponListKeyDown)
			]
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildLibraryFilterButtons()
{
	TSharedRef<SWrapBox> WrapBox = SNew(SWrapBox)
		.UseAllottedSize(true)
		.InnerSlotPadding(FVector2D(4.f, 4.f));
	WrapBox->AddSlot()
	[
		BuildLibraryFilterButton(
			EWeaponManagerLibraryCategory::Official,
			LOCTEXT("OfficialWeaponFilter", "正式武器"))
	];
	WrapBox->AddSlot()
	[
		BuildLibraryFilterButton(
			EWeaponManagerLibraryCategory::Test,
			LOCTEXT("TestWeaponFilter", "测试资产"))
	];
	return WrapBox;
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildLibraryFilterButton(
	EWeaponManagerLibraryCategory Category,
	const FText& Label)
{
	return SNew(SCheckBox)
		.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
		.IsChecked(this, &SWeaponManagerWidget::IsLibraryFilterChecked, Category)
		.OnCheckStateChanged(this, &SWeaponManagerWidget::HandleLibraryFilterChanged, Category)
		.ToolTipText(Category == EWeaponManagerLibraryCategory::Official
			? LOCTEXT("OfficialWeaponFilterTip", "显示 /Game/Code/Weapon 目录下的正式武器定义。")
			: LOCTEXT("TestWeaponFilterTip", "显示正式武器目录之外、带“测试”标记的武器定义。"))
		[
			SNew(STextBlock).Text(Label)
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildRightPanel()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 1.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (CurrentPage == EWeaponManagerPage::Skills && SelectedSkillType.IsValid())
						{
							return FText::Format(
								LOCTEXT("SkillInspectorTitle", "战技检查器 - {0}"),
								SelectedSkillType->DisplayName);
						}
						if (CurrentPage == EWeaponManagerPage::Actions)
						{
							return CurrentActionMode == EWeaponManagerActionMode::Basic
								? LOCTEXT("BasicInspector", "普通攻击数据检查器")
								: LOCTEXT("PassiveInspector", "被动反应数据检查器");
						}
						return LOCTEXT("WeaponInspector", "武器数据检查器");
					})
					.Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall")))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					DetailsView.ToSharedRef()
				]
				+ SScrollBox::Slot()
				.Padding(0.f, 8.f, 0.f, 0.f)
				[
					SAssignNew(SkillRequirementsBox, SBox)
				]
			]
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildCenterPanel()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderBackgroundColor(FLinearColor(0.055f, 0.055f, 0.055f, 1.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildPageTabs()
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(0.f, 8.f, 0.f, 0.f)
			[
				CurrentPage == EWeaponManagerPage::Actions
					? BuildActionPage()
					: CurrentPage == EWeaponManagerPage::Skills
						? BuildSkillPage()
						: BuildWeaponDetailsPage()
			]
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildPageTabs()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
		[
			SNew(SButton)
				.Text(LOCTEXT("ActionsTab", "动作配置"))
				.ButtonColorAndOpacity_Lambda([this]() { return PageButtonColor(CurrentPage == EWeaponManagerPage::Actions); })
				.OnClicked_Lambda([this]() { return SetPage(EWeaponManagerPage::Actions); })
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
		[
			SNew(SButton)
				.Text(LOCTEXT("SkillsTab", "战技"))
				.ButtonColorAndOpacity_Lambda([this]() { return PageButtonColor(CurrentPage == EWeaponManagerPage::Skills); })
				.OnClicked_Lambda([this]() { return SetPage(EWeaponManagerPage::Skills); })
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(SButton)
				.Text(LOCTEXT("DetailsTab", "武器详情"))
				.ButtonColorAndOpacity_Lambda([this]() { return PageButtonColor(CurrentPage == EWeaponManagerPage::Details); })
				.OnClicked_Lambda([this]() { return SetPage(EWeaponManagerPage::Details); })
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildActionPage()
{
	if (!GetSelectedWeapon())
	{
		return BuildEmptyState(
			LOCTEXT("NoWeaponForActions", "请选择武器"),
			LOCTEXT("NoWeaponForActionsDesc", "从左侧武器库选择一个武器定义，然后编辑它的动作蒙太奇。"));
	}

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);
	Root->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
		[
			SNew(SButton)
				.Text(LOCTEXT("BasicActions", "玩家基础动作"))
				.ButtonColorAndOpacity_Lambda([this]() { return PageButtonColor(CurrentActionMode == EWeaponManagerActionMode::Basic); })
				.OnClicked_Lambda([this]() { return SetActionMode(EWeaponManagerActionMode::Basic); })
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(SButton)
				.Text(LOCTEXT("PassiveActions", "被动反应"))
				.ButtonColorAndOpacity_Lambda([this]() { return PageButtonColor(CurrentActionMode == EWeaponManagerActionMode::Passive); })
				.OnClicked_Lambda([this]() { return SetActionMode(EWeaponManagerActionMode::Passive); })
		]
	];

	UAbilityData* ActionData = GetCurrentActionData(CurrentActionMode == EWeaponManagerActionMode::Passive);
	if (!ActionData)
	{
		const bool bPassive = CurrentActionMode == EWeaponManagerActionMode::Passive;
		Root->AddSlot().FillHeight(1.f).Padding(0.f, 8.f, 0.f, 0.f)
		[
			BuildEmptyState(
				bPassive ? LOCTEXT("MissingPassiveData", "尚未配置被动反应数据")
					: LOCTEXT("MissingAttackData", "尚未配置普通攻击数据"),
				bPassive
					? LOCTEXT("MissingPassiveDataDesc", "为受击、死亡和后续扩展的反应标签创建一份此武器专用的被动反应 DA。")
					: LOCTEXT("MissingAttackDataDesc", "为普通攻击、冲刺、装填和切换武器创建一份此武器专用的动作 DA。"),
				SNew(SButton)
					.Text(bPassive ? LOCTEXT("CreatePassiveData", "创建被动反应数据") : LOCTEXT("CreateAttackData", "创建普通攻击数据"))
					.OnClicked(bPassive ? FOnClicked::CreateSP(this, &SWeaponManagerWidget::CreatePassiveData)
						: FOnClicked::CreateSP(this, &SWeaponManagerWidget::CreateAttackData)))
		];
		return Root;
	}

	Root->AddSlot().FillHeight(1.f).Padding(0.f, 8.f, 0.f, 0.f)
	[
		CurrentActionMode == EWeaponManagerActionMode::Basic
			? BuildBasicActionRows()
			: BuildPassiveActionRows()
	];
	return Root;
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildBasicActionRows()
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
	[
		MakeSectionTitle(
			LOCTEXT("AttackSection", "普通攻击连段"),
			LOCTEXT("AttackSectionDesc", "未配置蒙太奇的动作槽位在运行时不会触发。"))
	];
	for (const FWeaponActionRow& Row : BasicAttackRows)
	{
		Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 3.f)
		[
			BuildActionRow(FText::FromString(Row.Label), Row.Tag, FText::FromString(Row.Description), false, Row.bRuntimeReady)
		];
	}

	Rows->AddSlot().AutoHeight().Padding(0.f, 9.f, 0.f, 5.f)
	[
		MakeSectionTitle(
			LOCTEXT("DashSection", "冲刺与冲刺攻击"),
			LOCTEXT("DashSectionDesc", "基础冲刺使用独立槽位；后续攻击使用四个冲刺攻击连段槽位。"))
	];
	for (const FWeaponActionRow& Row : DashRows)
	{
		Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 3.f)
		[
			BuildActionRow(FText::FromString(Row.Label), Row.Tag, FText::FromString(Row.Description), false, Row.bRuntimeReady)
		];
	}

	Rows->AddSlot().AutoHeight().Padding(0.f, 9.f, 0.f, 5.f)
	[
		MakeSectionTitle(
			LOCTEXT("UtilitySection", "武器通用动作"),
			LOCTEXT("UtilitySectionDesc", "装填通常只需要在远程武器上配置。"))
	];
	for (const FWeaponActionRow& Row : UtilityRows)
	{
		Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 3.f)
		[
			BuildActionRow(FText::FromString(Row.Label), Row.Tag, FText::FromString(Row.Description), false, Row.bRuntimeReady)
		];
	}

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			Rows
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildPassiveActionRows()
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
	[
		MakeSectionTitle(
			LOCTEXT("PassiveSection", "被动战斗反应"),
			LOCTEXT(
				"PassiveSectionDesc",
				"“已接入运行时”的行会被现有 GA 直接使用；“预留”行只固定未来扩展标签，不改变当前玩法。"))
	];
	for (const FWeaponActionRow& Row : PassiveRows)
	{
		Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 3.f)
		[
			BuildActionRow(FText::FromString(Row.Label), Row.Tag, FText::FromString(Row.Description), true, Row.bRuntimeReady)
		];
	}

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			Rows
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildActionRow(
	const FText& Label,
	const TCHAR* TagName,
	const FText& Description,
	bool bPassive,
	bool bRuntimeReady)
{
	const FGameplayTag ActionTag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
	const bool bConfigured = HasConfiguredMontage(ActionTag, bPassive);

	return SNew(SBorder)
		.Padding(FMargin(9.f, 6.f))
		.BorderBackgroundColor(bConfigured
			? FLinearColor(0.045f, 0.13f, 0.08f, 1.f)
			: FLinearColor(0.09f, 0.09f, 0.09f, 1.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.42f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
						.Text(Label)
						.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(TagName))
						.ToolTipText(Description)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
						.Text(bRuntimeReady
							? (bConfigured ? LOCTEXT("Ready", "已就绪") : LOCTEXT("Unavailable", "未配置蒙太奇，不可用"))
							: LOCTEXT("Reserved", "预留标签，运行时 GA 尚未接入"))
						.ColorAndOpacity(bRuntimeReady
							? (bConfigured ? FLinearColor(0.32f, 0.85f, 0.42f) : FLinearColor(0.85f, 0.52f, 0.2f))
							: FLinearColor(0.55f, 0.65f, 0.8f))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.58f).VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(SObjectPropertyEntryBox)
					.AllowedClass(UAnimMontage::StaticClass())
					.ObjectPath_Lambda([this, ActionTag, bPassive]() { return GetMontageObjectPath(ActionTag, bPassive); })
					.OnObjectChanged_Lambda([this, ActionTag, bPassive](const FAssetData& AssetData)
					{
						SetMontage(ActionTag, bPassive, AssetData);
					})
					.AllowClear(true)
					.DisplayThumbnail(true)
					.ThumbnailPool(ThumbnailPool)
			]
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildSkillPage()
{
	UWeaponDefinition* Weapon = GetSelectedWeapon();
	if (!Weapon)
	{
		return BuildEmptyState(
			LOCTEXT("NoWeaponForSkills", "请选择武器"),
			LOCTEXT("NoWeaponForSkillsDesc", "选择一个武器，然后管理它允许装备的战技和默认战技。"));
	}

	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
	[
		MakeSectionTitle(
			LOCTEXT("SkillCatalogue", "原生战技目录"),
			LOCTEXT(
				"SkillCatalogueDesc",
				"“创建”会为该武器生成此战技专属的战技 DA 和动作数据 DA；“移除”只从武器列表中解除引用，不会删除磁盘上的资产。"))
	];

	for (const FSkillTypePtr& SkillType : SkillTypes)
	{
		Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
		[
			BuildSkillRow(SkillType)
		];
	}

	if (SkillTypes.IsEmpty())
	{
		Rows->AddSlot().AutoHeight()
		[
			BuildEmptyState(
				LOCTEXT("NoSkillTypes", "没有找到已制作的原生战技"),
				LOCTEXT("NoSkillTypesDesc", "具体的 UGA_WeaponSkill C++ 类必须提供有效的 Weapon.Skill.* 标签。"))
		];
	}

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			Rows
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildSkillRow(FSkillTypePtr SkillType)
{
	UWeaponDefinition* Weapon = GetSelectedWeapon();
	UWeaponSkillDataAsset* Skill = SkillType.IsValid() ? FindWeaponSkill(SkillType->SkillTag) : nullptr;
	const bool bCreated = Skill != nullptr;
	const bool bDefault = Weapon && Weapon->DefaultWeaponSkill == Skill && Skill != nullptr;
	int32 MissingMontages = 0;
	if (Skill && Skill->AbilityData)
	{
		for (const FGameplayTag& Slot : SkillType->RequiredMontageSlots)
		{
			if (!Skill->AbilityData->GetMontage(Slot))
			{
				++MissingMontages;
			}
		}
	}

	return SNew(SBorder)
		.Padding(FMargin(9.f, 7.f))
		.BorderBackgroundColor(bCreated
			? FLinearColor(0.045f, 0.13f, 0.08f, 1.f)
			: FLinearColor(0.09f, 0.09f, 0.09f, 1.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SButton)
					.ButtonColorAndOpacity(FLinearColor::Transparent)
					.ToolTipText(SkillType->Description)
					.OnClicked_Lambda([this, SkillType]() { return SelectSkill(SkillType); })
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
								.Text(SkillType->DisplayName)
								.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
						[
							SNew(STextBlock)
								.Text(FText::FromString(SkillType->SkillTag.ToString()))
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
						[
							SNew(STextBlock)
								.Text(SkillType->Description)
								.AutoWrapText(true)
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
						[
							SNew(STextBlock)
								.Text_Lambda([bCreated, bDefault, MissingMontages]()
								{
									if (!bCreated)
									{
										return LOCTEXT("NotOwned", "此武器尚未拥有该战技");
									}
									if (MissingMontages > 0)
									{
										return FText::Format(
											LOCTEXT("MissingSkillMontages", "有 {0} 个必需蒙太奇槽位为空"),
											FText::AsNumber(MissingMontages));
									}
									return bDefault ? LOCTEXT("DefaultReadySkill", "默认战技 - 已就绪")
										: LOCTEXT("OwnedReadySkill", "已拥有 - 已就绪");
								})
								.ColorAndOpacity(!bCreated
									? FLinearColor(0.65f, 0.65f, 0.65f)
									: MissingMontages > 0
										? FLinearColor(0.85f, 0.52f, 0.2f)
										: FLinearColor(0.32f, 0.85f, 0.42f))
						]
					]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
					.Text(bCreated ? LOCTEXT("RemoveSkill", "移除") : LOCTEXT("CreateSkill", "创建"))
					.OnClicked_Lambda([this, SkillType, bCreated]()
					{
						return bCreated ? RemoveSkill(SkillType) : CreateSkill(SkillType);
					})
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
					.Text(bDefault ? LOCTEXT("IsDefault", "默认战技") : LOCTEXT("SetDefault", "设为默认"))
					.IsEnabled(bCreated && !bDefault)
					.OnClicked_Lambda([this, SkillType]() { return SetDefaultSkill(SkillType); })
			]
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildWeaponDetailsPage()
{
	UWeaponDefinition* Weapon = GetSelectedWeapon();
	if (!Weapon)
	{
		return BuildEmptyState(
			LOCTEXT("NoWeaponForDetails", "请选择武器"),
			LOCTEXT("NoWeaponForDetailsDesc", "选择一个武器，然后查看场景拾取、装备、模型和显示信息。"));
	}

	int32 Warnings = 0;
	const int32 Errors = CountWeaponIssues(Weapon, Warnings);
	const FText WeaponTypeText = StaticEnum<EWeaponType>()->GetDisplayNameTextByValue(static_cast<int64>(Weapon->WeaponType));

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 7.f)
			[
				MakeSectionTitle(
					LOCTEXT("GeneralOverview", "武器概览"),
					LOCTEXT("GeneralOverviewDesc", "右侧检查器可编辑完整的场景拾取、装备、模型、远程弹丸和显示信息字段。"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
			[
				SNew(SBorder).Padding(10.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
							.Text(FText::Format(LOCTEXT("AssetPathFmt", "资产路径：{0}"), FText::FromString(Weapon->GetPathName())))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[
						SNew(STextBlock)
							.Text(FText::Format(LOCTEXT("TypeFmt", "武器类型：{0}"), WeaponTypeText))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[
						SNew(STextBlock)
							.Text(FText::Format(
								LOCTEXT("SkillCountFmt", "可装备战技：{0} | 默认战技：{1}"),
								FText::AsNumber(Weapon->AvailableWeaponSkills.Num()),
								FText::FromString(GetNameSafe(Weapon->ResolveDefaultWeaponSkill()))))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[
						SNew(STextBlock)
							.Text(FText::Format(
								LOCTEXT("ValidationFmt", "校验：{0} 个错误，{1} 个警告"),
								FText::AsNumber(Errors),
								FText::AsNumber(Warnings)))
							.ColorAndOpacity(Errors > 0
								? FLinearColor(0.95f, 0.28f, 0.22f)
								: Warnings > 0
									? FLinearColor(0.85f, 0.52f, 0.2f)
									: FLinearColor(0.32f, 0.85f, 0.42f))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 7.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 5.f, 0.f)
				[
					SNew(SButton)
						.Text(LOCTEXT("OpenWeaponAsset", "打开武器资产"))
						.OnClicked_Lambda([this, Weapon]() { return OpenAsset(Weapon); })
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 5.f, 0.f)
				[
					SNew(SButton)
						.Text(LOCTEXT("BrowseWeaponAsset", "在内容浏览器中定位"))
						.OnClicked_Lambda([this, Weapon]() { return SyncAsset(Weapon); })
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
						.Text(LOCTEXT("OpenWeaponInfo", "打开显示信息"))
						.IsEnabled(Weapon->WeaponInfo != nullptr)
						.OnClicked_Lambda([this, Weapon]() { return OpenAsset(Weapon->WeaponInfo); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				MakeSectionTitle(
					LOCTEXT("GeneralFieldGuide", "字段分组说明"),
					LOCTEXT(
						"GeneralFieldGuideDesc",
						"“场景拾取”控制拖入关卡后的显示模型和变换；“装备”控制生成的武器 Actor、挂接插槽、动画层和远程弹丸；“显示信息”控制名称、简介、缩略图和激活区图片。"))
			]
		];
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildSelectedSkillRequirements()
{
	if (CurrentPage != EWeaponManagerPage::Skills || !SelectedSkillType.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	UWeaponSkillDataAsset* Skill = FindWeaponSkill(SelectedSkillType->SkillTag);
	if (!Skill)
	{
		return SNullWidget::NullWidget;
	}

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);
	Root->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
	[
		MakeSectionTitle(
			LOCTEXT("SkillRuntimeContract", "GA 声明的运行时字段"),
			LOCTEXT(
				"SkillRuntimeContractDesc",
				"这里只显示该原生 GA 明确声明的蒙太奇槽位；战技独有的数值字段继续在上方专属 DA 详情中编辑。"))
	];
	Root->AddSlot().AutoHeight().Padding(2.f, 0.f, 2.f, 2.f)
	[
		SNew(STextBlock)
			.Text(SelectedSkillType->Description)
			.AutoWrapText(true)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
	];
	Root->AddSlot().AutoHeight().Padding(2.f, 5.f, 2.f, 2.f)
	[
		SNew(STextBlock)
			.Text(FText::Format(
				LOCTEXT("SkillGAClassFmt", "GA 类：{0}"),
				Skill->AbilityClass
					? Skill->AbilityClass->GetDisplayNameText()
					: LOCTEXT("MissingSkillGAClass", "未配置")))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
	];
	Root->AddSlot().AutoHeight().Padding(2.f, 0.f, 2.f, 7.f)
	[
		SNew(STextBlock)
			.Text(FText::Format(
				LOCTEXT("SkillDAClassFmt", "DA 类型：{0}"),
				Skill->GetClass()->GetDisplayNameText()))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
	];

	if (SelectedSkillType->RequiredMontageSlots.IsEmpty())
	{
		Root->AddSlot().AutoHeight().Padding(2.f, 4.f)
		[
			SNew(STextBlock)
				.Text(LOCTEXT("NoRequiredSkillMontages", "此 GA 没有声明必需的蒙太奇槽位。"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
	}
	else
	{
		for (const FGameplayTag& MontageSlot : SelectedSkillType->RequiredMontageSlots)
		{
			Root->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SBorder)
					.Padding(FMargin(8.f, 6.f))
					.BorderBackgroundColor(GetSkillMontageObjectPath(MontageSlot).IsEmpty()
						? FLinearColor(0.09f, 0.09f, 0.09f, 1.f)
						: FLinearColor(0.045f, 0.13f, 0.08f, 1.f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
									.Text(GetWeaponSkillMontageSlotLabel(SelectedSkillType->SkillTag, MontageSlot))
									.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
							[
								SNew(STextBlock)
									.Text(FText::FromString(MontageSlot.ToString()))
									.ColorAndOpacity(FSlateColor::UseSubduedForeground())
							]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SObjectPropertyEntryBox)
								.AllowedClass(UAnimMontage::StaticClass())
								.ObjectPath_Lambda([this, MontageSlot]()
								{
									return GetSkillMontageObjectPath(MontageSlot);
								})
								.OnObjectChanged_Lambda([this, MontageSlot](const FAssetData& AssetData)
								{
									SetSkillMontage(MontageSlot, AssetData);
								})
								.AllowClear(true)
								.DisplayThumbnail(true)
								.ThumbnailPool(ThumbnailPool)
						]
					]
			];
		}
	}

	Root->AddSlot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
	[
		SNew(SButton)
			.Text(LOCTEXT("OpenAdvancedSkillAbilityData", "打开高级动作数据"))
			.ToolTipText(LOCTEXT(
				"OpenAdvancedSkillAbilityDataTip",
				"打开底层动作数据 DA，用于配置上下文分支和其他高级选项。"))
			.IsEnabled(Skill->AbilityData != nullptr)
			.OnClicked_Lambda([this, Skill]()
			{
				return OpenAsset(Skill->AbilityData);
			})
	];

	return Root;
}

TSharedRef<SWidget> SWeaponManagerWidget::BuildEmptyState(
	const FText& Title,
	const FText& Description,
	TSharedPtr<SWidget> ActionWidget)
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	Box->AddSlot().AutoHeight().HAlign(HAlign_Center)
	[
		SNew(STextBlock)
			.Text(Title)
			.Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall")))
	];
	Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(12.f, 6.f, 12.f, 0.f)
	[
		SNew(STextBlock)
			.Text(Description)
			.AutoWrapText(true)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
	];
	if (ActionWidget.IsValid())
	{
		Box->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 12.f, 0.f, 0.f)
		[
			ActionWidget.ToSharedRef()
		];
	}

	return SNew(SBorder)
		.Padding(24.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			Box
		];
}

void SWeaponManagerWidget::RefreshWeapons(bool bKeepSelection)
{
	const FName PreviousPackage = bKeepSelection && SelectedWeaponItem.IsValid()
		? SelectedWeaponItem->AssetData.PackageName
		: NAME_None;

	AllWeapons.Reset();
	SelectedWeaponItem.Reset();

	IAssetRegistry& Registry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.ScanPathsSynchronous({TEXT("/Game/Code/Weapon")}, false);

	FARFilter Filter;
	Filter.ClassPaths.Add(UWeaponDefinition::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	TArray<FAssetData> Assets;
	Registry.GetAssets(Filter, Assets);
	Assets.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.PackageName.LexicalLess(B.PackageName);
	});

	for (const FAssetData& AssetData : Assets)
	{
		UWeaponDefinition* Weapon = Cast<UWeaponDefinition>(AssetData.GetAsset());
		if (!Weapon)
		{
			continue;
		}

		FWeaponItemPtr Item = MakeShared<FWeaponManagerWeaponItem>();
		Item->AssetData = AssetData;
		Item->Weapon = Weapon;
		Item->LibraryCategory = IsOfficialWeaponPackage(AssetData.PackageName.ToString())
			? EWeaponManagerLibraryCategory::Official
			: EWeaponManagerLibraryCategory::Test;
		UObject* ThumbnailObject = Weapon->WeaponInfo && Weapon->WeaponInfo->Thumbnail
			? static_cast<UObject*>(Weapon->WeaponInfo->Thumbnail.Get())
			: static_cast<UObject*>(Weapon);
		Item->Thumbnail = MakeShared<FAssetThumbnail>(ThumbnailObject, 56, 56, ThumbnailPool);
		Item->DisplayName = Weapon->WeaponInfo && !Weapon->WeaponInfo->WeaponName.IsEmpty()
			? Weapon->WeaponInfo->WeaponName
			: FText::FromName(AssetData.AssetName);
		Item->WeaponTypeText =
			StaticEnum<EWeaponType>()->GetDisplayNameTextByValue(static_cast<int64>(Weapon->WeaponType));
		int32 Warnings = 0;
		const int32 Errors = CountWeaponIssues(Weapon, Warnings);
		Item->ValidationText = GetWeaponValidationText(Weapon);
		Item->ValidationColor = Errors > 0
			? FSlateColor(FLinearColor(0.95f, 0.28f, 0.22f))
			: Warnings > 0
				? FSlateColor(FLinearColor(0.85f, 0.52f, 0.2f))
				: FSlateColor(FLinearColor(0.32f, 0.85f, 0.42f));
		Item->SearchText = FString::Printf(
			TEXT("%s %s %s %s"),
			*AssetData.AssetName.ToString(),
			*AssetData.PackageName.ToString(),
			*StaticEnum<EWeaponType>()->GetNameStringByValue(static_cast<int64>(Weapon->WeaponType)),
			Item->LibraryCategory == EWeaponManagerLibraryCategory::Official
				? TEXT("official 正式")
				: TEXT("test 测试"));
		AllWeapons.Add(Item);
		if (AssetData.PackageName == PreviousPackage)
		{
			SelectedWeaponItem = Item;
		}
	}

	RebuildFilteredWeapons();
	if (!SelectedWeaponItem.IsValid() && !FilteredWeapons.IsEmpty())
	{
		SelectedWeaponItem = FilteredWeapons[0];
	}
	if (WeaponListView.IsValid())
	{
		WeaponListView->RequestListRefresh();
		if (SelectedWeaponItem.IsValid())
		{
			WeaponListView->SetSelection(SelectedWeaponItem);
			WeaponListView->RequestScrollIntoView(SelectedWeaponItem);
		}
	}
	RefreshStatus();
}

void SWeaponManagerWidget::RefreshSkillTypes()
{
	SkillTypes.Reset();
	TSet<UClass*> CandidateClasses;

	// GetDerivedClasses is the normal clean-editor path. TObjectIterator is
	// also consulted because the derived-class cache can lag behind a native
	// hot reload even though the replacement UClasses are already registered.
	TArray<UClass*> DerivedClasses;
	GetDerivedClasses(UGA_WeaponSkill::StaticClass(), DerivedClasses, true);
	CandidateClasses.Append(DerivedClasses);
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;
		if (Class && Class != UGA_WeaponSkill::StaticClass() && Class->IsChildOf(UGA_WeaponSkill::StaticClass()))
		{
			CandidateClasses.Add(Class);
		}
	}

	TMap<FGameplayTag, FSkillTypePtr> SkillTypeByTag;
	for (UClass* Class : CandidateClasses)
	{
		if (!Class
			|| !Class->HasAnyClassFlags(CLASS_Native)
			|| Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
		{
			continue;
		}
		const UGA_WeaponSkill* CDO = Class->GetDefaultObject<UGA_WeaponSkill>();
		if (!CDO || !CDO->GetWeaponSkillTag().IsValid())
		{
			continue;
		}

		FSkillTypePtr Type = MakeShared<FWeaponManagerSkillType>();
		Type->SkillTag = CDO->GetWeaponSkillTag();
		Type->DisplayName = CDO->GetWeaponSkillDisplayName().IsEmpty()
			? FText::FromString(Class->GetName())
			: CDO->GetWeaponSkillDisplayName();
		Type->Description = CDO->GetWeaponSkillDescription();
		Type->AbilityClass = Class;
		Type->DataAssetClass = CDO->GetWeaponSkillDataClass().LoadSynchronous();
		if (!Type->DataAssetClass)
		{
			Type->DataAssetClass = UWeaponSkillDataAsset::StaticClass();
		}
		Type->RequiredMontageSlots = CDO->GetRequiredMontageSlots();

		if (const FSkillTypePtr* Existing = SkillTypeByTag.Find(Type->SkillTag))
		{
			const bool bExistingIsSuperseded = (*Existing)->AbilityClass
				&& (*Existing)->AbilityClass->HasAnyClassFlags(CLASS_NewerVersionExists);
			const bool bCandidateIsSuperseded = Class->HasAnyClassFlags(CLASS_NewerVersionExists);
			if (!bExistingIsSuperseded || bCandidateIsSuperseded)
			{
				continue;
			}
		}
		SkillTypeByTag.Add(Type->SkillTag, Type);
	}

	SkillTypeByTag.GenerateValueArray(SkillTypes);
	SkillTypes.Sort([](const FSkillTypePtr& A, const FSkillTypePtr& B)
	{
		return A->SkillTag.ToString() < B->SkillTag.ToString();
	});
}

void SWeaponManagerWidget::RebuildFilteredWeapons()
{
	FilteredWeapons.Reset();
	for (const FWeaponItemPtr& Item : AllWeapons)
	{
		if (!Item.IsValid())
		{
			continue;
		}
		if (!EnabledLibraryFilters.Contains(Item->LibraryCategory))
		{
			continue;
		}
		if (SearchFilter.IsEmpty() || Item->SearchText.Contains(SearchFilter, ESearchCase::IgnoreCase))
		{
			FilteredWeapons.Add(Item);
		}
	}
	if (WeaponListView.IsValid())
	{
		if (!FilteredWeapons.Contains(SelectedWeaponItem))
		{
			SelectedWeaponItem = FilteredWeapons.IsEmpty() ? nullptr : FilteredWeapons[0];
			WeaponListView->SetSelection(SelectedWeaponItem);
		}
		WeaponListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SWeaponManagerWidget::GenerateWeaponRow(
	FWeaponItemPtr Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FWeaponItemPtr>, OwnerTable)
		.Padding(FMargin(3.f, 3.f))
		[
			SNew(SWeaponManagerDragCard)
				.Item(Item)
				.OwnerWidget(SharedThis(this))
		];
}

void SWeaponManagerWidget::SelectWeaponForInteraction(
	TSharedPtr<FWeaponManagerWeaponItem> Item)
{
	if (!Item.IsValid())
	{
		return;
	}

	if (WeaponListView.IsValid())
	{
		WeaponListView->SetSelection(Item, ESelectInfo::OnMouseClick);
		FSlateApplication::Get().SetKeyboardFocus(
			WeaponListView,
			EFocusCause::Mouse);
	}
	HandleWeaponSelectionChanged(Item, ESelectInfo::OnMouseClick);
}

FReply SWeaponManagerWidget::BeginWeaponDrag(
	TSharedPtr<FWeaponManagerWeaponItem> Item)
{
	SelectWeaponForInteraction(Item);
	UWeaponDefinition* Weapon = Item.IsValid() ? Item->Weapon.Get() : nullptr;
	if (!Weapon)
	{
		SetStatus(LOCTEXT("DragInvalidWeapon", "无法将无效的武器定义拖入关卡。"), true);
		return FReply::Unhandled();
	}
	if (!Weapon->DisplayMesh)
	{
		SetStatus(
			LOCTEXT(
				"DragWeaponMissingDisplayMesh",
				"拖入关卡前，请先在此武器定义中配置“场景拾取 > 显示模型 > 场景显示网格”。"),
			true);
		return FReply::Unhandled();
	}

	UWeaponDefinitionActorFactory* Factory =
		NewObject<UWeaponDefinitionActorFactory>(GetTransientPackage());
	Factory->Configure(Weapon);
	ActiveDragFactoryRoot = TStrongObjectPtr<UObject>(Factory);
	SetStatus(FText::Format(
		LOCTEXT("DraggingWeaponStatus", "正在拖拽 {0}：在关卡视口中松开鼠标即可放置 BP_WeaponSpawner。"),
		Item->DisplayName));
	return FReply::Handled().BeginDragDrop(FAssetDragDropOp::New(Item->AssetData, Factory));
}

void SWeaponManagerWidget::HandleWeaponSelectionChanged(FWeaponItemPtr Item, ESelectInfo::Type SelectInfo)
{
	if (!Item.IsValid())
	{
		return;
	}
	SelectedWeaponItem = Item;
	SelectedSkillType.Reset();
	RebuildCenterPanel();
	RefreshDetailsPanel();
	RefreshStatus();
}

void SWeaponManagerWidget::HandleSearchChanged(const FText& NewText)
{
	SearchFilter = NewText.ToString();
	RebuildFilteredWeapons();
}

FReply SWeaponManagerWidget::HandleWeaponListKeyDown(
	const FGeometry& Geometry,
	const FKeyEvent& KeyEvent)
{
	if (KeyEvent.IsControlDown()
		&& !KeyEvent.IsAltDown()
		&& KeyEvent.GetKey() == EKeys::B
		&& SelectedWeaponItem.IsValid()
		&& SelectedWeaponItem->AssetData.IsValid()
		&& GEditor)
	{
		TArray<FAssetData> AssetsToSync;
		AssetsToSync.Add(SelectedWeaponItem->AssetData);
		GEditor->SyncBrowserToObjects(AssetsToSync, true);
		SetStatus(FText::Format(
			LOCTEXT("SyncedWeaponToContentBrowser", "已在内容浏览器中定位 {0}。"),
			SelectedWeaponItem->DisplayName));
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SWeaponManagerWidget::HandleLibraryFilterChanged(
	ECheckBoxState NewState,
	EWeaponManagerLibraryCategory Category)
{
	if (NewState == ECheckBoxState::Checked)
	{
		EnabledLibraryFilters.Add(Category);
	}
	else
	{
		EnabledLibraryFilters.Remove(Category);
	}

	RebuildFilteredWeapons();
	RebuildCenterPanel();
	RefreshDetailsPanel();
	RefreshStatus();
}

ECheckBoxState SWeaponManagerWidget::IsLibraryFilterChecked(
	EWeaponManagerLibraryCategory Category) const
{
	return EnabledLibraryFilters.Contains(Category)
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
}

void SWeaponManagerWidget::RebuildCenterPanel()
{
	if (CenterPanelBox.IsValid())
	{
		CenterPanelBox->SetContent(BuildCenterPanel());
	}
}

void SWeaponManagerWidget::RefreshDetailsPanel()
{
	if (!DetailsView.IsValid())
	{
		return;
	}

	if (SkillRequirementsBox.IsValid())
	{
		SkillRequirementsBox->SetContent(BuildSelectedSkillRequirements());
	}
	UWeaponDefinition* Weapon = GetSelectedWeapon();
	if (!Weapon)
	{
		DetailsView->SetObject(nullptr, true);
		return;
	}

	if (CurrentPage == EWeaponManagerPage::Actions)
	{
		DetailsView->SetObject(
			GetCurrentActionData(CurrentActionMode == EWeaponManagerActionMode::Passive),
			true);
		return;
	}
	if (CurrentPage == EWeaponManagerPage::Skills && SelectedSkillType.IsValid())
	{
		if (UWeaponSkillDataAsset* Skill = FindWeaponSkill(SelectedSkillType->SkillTag))
		{
			DetailsView->SetObject(Skill, true);
			return;
		}
	}
	DetailsView->SetObject(Weapon, true);
}

FReply SWeaponManagerWidget::SetPage(EWeaponManagerPage NewPage)
{
	CurrentPage = NewPage;
	RebuildCenterPanel();
	RefreshDetailsPanel();
	return FReply::Handled();
}

FReply SWeaponManagerWidget::SetActionMode(EWeaponManagerActionMode NewMode)
{
	CurrentActionMode = NewMode;
	RebuildCenterPanel();
	RefreshDetailsPanel();
	return FReply::Handled();
}

UWeaponDefinition* SWeaponManagerWidget::GetSelectedWeapon() const
{
	return SelectedWeaponItem.IsValid() ? SelectedWeaponItem->Weapon.Get() : nullptr;
}

UWeaponSkillDataAsset* SWeaponManagerWidget::FindWeaponSkill(const FGameplayTag& SkillTag) const
{
	const UWeaponDefinition* Weapon = GetSelectedWeapon();
	if (!Weapon || !SkillTag.IsValid())
	{
		return nullptr;
	}
	for (UWeaponSkillDataAsset* Skill : Weapon->AvailableWeaponSkills)
	{
		if (Skill && Skill->GetResolvedSkillTag().MatchesTagExact(SkillTag))
		{
			return Skill;
		}
	}
	return nullptr;
}

UAbilityData* SWeaponManagerWidget::GetCurrentActionData(bool bPassive) const
{
	UWeaponDefinition* Weapon = GetSelectedWeapon();
	if (!Weapon)
	{
		return nullptr;
	}
	return bPassive
		? static_cast<UAbilityData*>(Weapon->PassiveAbilityData.Get())
		: static_cast<UAbilityData*>(Weapon->AttackAbilityData.Get());
}

UAnimMontage* SWeaponManagerWidget::GetMontage(const FGameplayTag& ActionTag, bool bPassive) const
{
	UAbilityData* Data = GetCurrentActionData(bPassive);
	if (!Data || !ActionTag.IsValid())
	{
		return nullptr;
	}
	if (bPassive)
	{
		const FPassiveActionData* Entry = Data->PassiveMap.Find(ActionTag);
		return Entry ? Entry->Montage.Get() : nullptr;
	}
	const TObjectPtr<UAnimMontage>* Entry = Data->MontageMap.Find(ActionTag);
	return Entry ? Entry->Get() : nullptr;
}

void SWeaponManagerWidget::SetMontage(const FGameplayTag& ActionTag, bool bPassive, const FAssetData& AssetData)
{
	UAbilityData* Data = GetCurrentActionData(bPassive);
	if (!Data || !ActionTag.IsValid())
	{
		return;
	}
	UAnimMontage* NewMontage = Cast<UAnimMontage>(AssetData.GetAsset());
	const FScopedTransaction Transaction(LOCTEXT("SetWeaponMontageTx", "设置武器动作蒙太奇"));
	Data->Modify();
	if (bPassive)
	{
		FPassiveActionData& Entry = Data->PassiveMap.FindOrAdd(ActionTag);
		Entry.Montage = NewMontage;
	}
	else
	{
		Data->MontageMap.FindOrAdd(ActionTag) = NewMontage;
	}
	Data->MarkPackageDirty();
	SetStatus(FText::Format(
		LOCTEXT("SetMontageStatus", "{0}：已设置为 {1}。资产尚未保存，完成后请点击“签出并保存”。"),
		FText::FromString(ActionTag.ToString()),
		FText::FromString(GetNameSafe(NewMontage))));
	RebuildCenterPanel();
	RefreshDetailsPanel();
}

FString SWeaponManagerWidget::GetMontageObjectPath(FGameplayTag ActionTag, bool bPassive) const
{
	if (UAnimMontage* Montage = GetMontage(ActionTag, bPassive))
	{
		return Montage->GetPathName();
	}
	return FString();
}

bool SWeaponManagerWidget::HasConfiguredMontage(const FGameplayTag& ActionTag, bool bPassive) const
{
	return GetMontage(ActionTag, bPassive) != nullptr;
}

void SWeaponManagerWidget::SetSkillMontage(const FGameplayTag& MontageSlot, const FAssetData& AssetData)
{
	if (!SelectedSkillType.IsValid() || !MontageSlot.IsValid())
	{
		return;
	}

	UWeaponSkillDataAsset* Skill = FindWeaponSkill(SelectedSkillType->SkillTag);
	UWeaponSkillAbilityMontageData* AbilityData = Skill ? Skill->AbilityData.Get() : nullptr;
	if (!AbilityData)
	{
		return;
	}

	UAnimMontage* NewMontage = Cast<UAnimMontage>(AssetData.GetAsset());
	const FScopedTransaction Transaction(LOCTEXT("SetWeaponSkillMontageTx", "设置武器战技蒙太奇"));
	AbilityData->Modify();
	AbilityData->MontageMap.FindOrAdd(MontageSlot) = NewMontage;
	AbilityData->MarkPackageDirty();
	SetStatus(FText::Format(
		LOCTEXT("SetSkillMontageStatus", "{0}：已设置为 {1}。资产尚未保存，完成后请点击“签出并保存”。"),
		FText::FromString(MontageSlot.ToString()),
		FText::FromString(GetNameSafe(NewMontage))));
	RebuildCenterPanel();
	RefreshDetailsPanel();
}

FString SWeaponManagerWidget::GetSkillMontageObjectPath(FGameplayTag MontageSlot) const
{
	if (!SelectedSkillType.IsValid() || !MontageSlot.IsValid())
	{
		return FString();
	}
	const UWeaponSkillDataAsset* Skill = FindWeaponSkill(SelectedSkillType->SkillTag);
	const UWeaponSkillAbilityMontageData* AbilityData = Skill ? Skill->AbilityData.Get() : nullptr;
	if (!AbilityData)
	{
		return FString();
	}
	if (const TObjectPtr<UAnimMontage>* Montage = AbilityData->MontageMap.Find(MontageSlot))
	{
		return *Montage ? (*Montage)->GetPathName() : FString();
	}
	return FString();
}

FReply SWeaponManagerWidget::SelectSkill(FSkillTypePtr SkillType)
{
	SelectedSkillType = SkillType;
	RefreshDetailsPanel();
	SetStatus(FindWeaponSkill(SkillType->SkillTag)
		? FText::Format(LOCTEXT("SelectedSkill", "已选择 {0}，可在右侧编辑它的专属 DA。"), SkillType->DisplayName)
		: FText::Format(LOCTEXT("SelectedEmptySkill", "此武器尚未拥有 {0}，点击“创建”即可添加。"), SkillType->DisplayName));
	return FReply::Handled();
}

FReply SWeaponManagerWidget::CreateSkill(FSkillTypePtr SkillType)
{
	UWeaponDefinition* Weapon = GetSelectedWeapon();
	if (!Weapon || !SkillType.IsValid() || !SkillType->AbilityClass || FindWeaponSkill(SkillType->SkillTag))
	{
		return FReply::Handled();
	}

	const FString Folder = GetWeaponFolder() / TEXT("WeaponSkills");
	const FString Leaf = ObjectTools::SanitizeObjectName(SkillType->SkillTag.GetTagName().ToString().RightChop(
		SkillType->SkillTag.GetTagName().ToString().Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromEnd) + 1));
	const FString Stem = GetWeaponStem();
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();

	FString SkillPackageName;
	FString SkillAssetName;
	AssetTools.CreateUniqueAssetName(Folder / FString::Printf(TEXT("DA_WS_%s_%s"), *Stem, *Leaf), TEXT(""), SkillPackageName, SkillAssetName);
	FString AbilityDataPackageName;
	FString AbilityDataAssetName;
	AssetTools.CreateUniqueAssetName(Folder / FString::Printf(TEXT("DA_WSA_%s_%s"), *Stem, *Leaf), TEXT(""), AbilityDataPackageName, AbilityDataAssetName);

	TArray<UPackage*> Packages;
	UClass* SkillDataClass = SkillType->DataAssetClass
		? SkillType->DataAssetClass.Get()
		: UWeaponSkillDataAsset::StaticClass();
	UWeaponSkillDataAsset* Skill = Cast<UWeaponSkillDataAsset>(
		CreateAsset(SkillDataClass, SkillPackageName, SkillAssetName, Packages));
	UWeaponSkillAbilityMontageData* AbilityData = Cast<UWeaponSkillAbilityMontageData>(
		CreateAsset(UWeaponSkillAbilityMontageData::StaticClass(), AbilityDataPackageName, AbilityDataAssetName, Packages));
	if (!Skill || !AbilityData)
	{
		SetStatus(LOCTEXT("CreateSkillFailed", "创建武器战技资产失败。"), true);
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateWeaponSkillTx", "创建武器战技"));
	Weapon->Modify();
	Skill->Modify();
	Skill->SkillTag = SkillType->SkillTag;
	Skill->DisplayName = SkillType->DisplayName;
	Skill->Description = SkillType->Description;
	Skill->AbilityClass = SkillType->AbilityClass;
	Skill->AbilityData = AbilityData;
	Weapon->AvailableWeaponSkills.Add(Skill);
	if (!Weapon->DefaultWeaponSkill)
	{
		Weapon->DefaultWeaponSkill = Skill;
	}
	Weapon->MarkPackageDirty();
	Skill->MarkPackageDirty();
	AbilityData->MarkPackageDirty();
	Packages.AddUnique(Weapon->GetOutermost());

	const bool bSaved = SavePackages(Packages, true);
	SelectedSkillType = SkillType;
	SetStatus(
		bSaved
			? FText::Format(
				LOCTEXT("CreatedSkillStatus", "已为 {0} 创建专属 GA/DA 配置，并加入当前武器。"),
				SkillType->DisplayName)
			: FText::Format(
				LOCTEXT(
					"CreatedSkillUnsavedStatus",
					"已在内存中创建 {0}，但签出/保存失败或被取消。关闭编辑器前请重新点击“签出并保存”。"),
				SkillType->DisplayName),
		!bSaved);
	RebuildCenterPanel();
	RefreshDetailsPanel();
	return FReply::Handled();
}

FReply SWeaponManagerWidget::RemoveSkill(FSkillTypePtr SkillType)
{
	UWeaponDefinition* Weapon = GetSelectedWeapon();
	UWeaponSkillDataAsset* Skill = SkillType.IsValid() ? FindWeaponSkill(SkillType->SkillTag) : nullptr;
	if (!Weapon || !Skill)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("RemoveWeaponSkillTx", "从武器移除战技"));
	Weapon->Modify();
	Weapon->AvailableWeaponSkills.Remove(Skill);
	if (Weapon->DefaultWeaponSkill == Skill)
	{
		Weapon->DefaultWeaponSkill = nullptr;
		for (UWeaponSkillDataAsset* Candidate : Weapon->AvailableWeaponSkills)
		{
			if (Candidate && Candidate->AbilityClass && Candidate->AbilityData)
			{
				Weapon->DefaultWeaponSkill = Candidate;
				break;
			}
		}
	}
	Weapon->MarkPackageDirty();
	SelectedSkillType = SkillType;
	SetStatus(FText::Format(
		LOCTEXT("RemovedSkillStatus", "已从当前武器移除 {0}；磁盘上的 DA 资产没有删除。"),
		SkillType->DisplayName));
	RebuildCenterPanel();
	RefreshDetailsPanel();
	return FReply::Handled();
}

FReply SWeaponManagerWidget::SetDefaultSkill(FSkillTypePtr SkillType)
{
	UWeaponDefinition* Weapon = GetSelectedWeapon();
	UWeaponSkillDataAsset* Skill = SkillType.IsValid() ? FindWeaponSkill(SkillType->SkillTag) : nullptr;
	if (!Weapon || !Skill)
	{
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(LOCTEXT("SetDefaultWeaponSkillTx", "设置默认武器战技"));
	Weapon->Modify();
	Weapon->DefaultWeaponSkill = Skill;
	Weapon->MarkPackageDirty();
	SetStatus(FText::Format(LOCTEXT("DefaultSkillStatus", "{0} 现已设为该武器唯一的默认装备战技。"), SkillType->DisplayName));
	RebuildCenterPanel();
	RefreshDetailsPanel();
	return FReply::Handled();
}

FReply SWeaponManagerWidget::CreateAttackData()
{
	UWeaponDefinition* Weapon = GetSelectedWeapon();
	if (!Weapon || Weapon->AttackAbilityData)
	{
		return FReply::Handled();
	}
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	FString PackageName;
	FString AssetName;
	AssetTools.CreateUniqueAssetName(
		GetWeaponFolder() / FString::Printf(TEXT("DA_Attack_%s"), *GetWeaponStem()),
		TEXT(""),
		PackageName,
		AssetName);
	TArray<UPackage*> Packages;
	UWeaponAttackAbilityMontageData* Data = Cast<UWeaponAttackAbilityMontageData>(
		CreateAsset(UWeaponAttackAbilityMontageData::StaticClass(), PackageName, AssetName, Packages));
	if (!Data)
	{
		SetStatus(LOCTEXT("CreateAttackFailed", "创建普通攻击数据失败。"), true);
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(LOCTEXT("CreateAttackDataTx", "创建武器普通攻击数据"));
	Weapon->Modify();
	Weapon->AttackAbilityData = Data;
	Weapon->MarkPackageDirty();
	Packages.AddUnique(Weapon->GetOutermost());
	const bool bSaved = SavePackages(Packages, true);
	SetStatus(
		bSaved
			? LOCTEXT("CreatedAttackDataStatus", "已创建并分配此武器专用的普通攻击数据。")
			: LOCTEXT(
				"CreatedAttackDataUnsavedStatus",
				"普通攻击数据已在内存中创建，但签出/保存失败或被取消。关闭编辑器前请重新点击“签出并保存”。"),
		!bSaved);
	RebuildCenterPanel();
	RefreshDetailsPanel();
	return FReply::Handled();
}

FReply SWeaponManagerWidget::CreatePassiveData()
{
	UWeaponDefinition* Weapon = GetSelectedWeapon();
	if (!Weapon || Weapon->PassiveAbilityData)
	{
		return FReply::Handled();
	}
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	FString PackageName;
	FString AssetName;
	AssetTools.CreateUniqueAssetName(
		GetWeaponFolder() / FString::Printf(TEXT("DA_Passive_%s"), *GetWeaponStem()),
		TEXT(""),
		PackageName,
		AssetName);
	TArray<UPackage*> Packages;
	UWeaponPassiveAbilityMontageData* Data = Cast<UWeaponPassiveAbilityMontageData>(
		CreateAsset(UWeaponPassiveAbilityMontageData::StaticClass(), PackageName, AssetName, Packages));
	if (!Data)
	{
		SetStatus(LOCTEXT("CreatePassiveFailed", "创建被动反应数据失败。"), true);
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(LOCTEXT("CreatePassiveDataTx", "创建武器被动反应数据"));
	Weapon->Modify();
	Weapon->PassiveAbilityData = Data;
	Weapon->MarkPackageDirty();
	Packages.AddUnique(Weapon->GetOutermost());
	const bool bSaved = SavePackages(Packages, true);
	SetStatus(
		bSaved
			? LOCTEXT("CreatedPassiveDataStatus", "已创建并分配此武器专用的被动反应数据。")
			: LOCTEXT(
				"CreatedPassiveDataUnsavedStatus",
				"被动反应数据已在内存中创建，但签出/保存失败或被取消。关闭编辑器前请重新点击“签出并保存”。"),
		!bSaved);
	RebuildCenterPanel();
	RefreshDetailsPanel();
	return FReply::Handled();
}

FReply SWeaponManagerWidget::OpenCreateWeaponDialog()
{
	PendingNewWeaponName = TEXT("NewWeapon");
	PendingNewWeaponFolder = TEXT("/Game/Code/Weapon/NewWeapon");

	TSharedRef<SWindow> Dialog = SNew(SWindow)
		.Title(LOCTEXT("NewWeaponDialogTitle", "创建武器"))
		.ClientSize(FVector2D(520.f, 235.f))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	Dialog->SetContent(
		SNew(SBorder)
		.Padding(16.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("WeaponNameLabel", "武器名称"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 10.f)
			[
				SNew(SEditableTextBox)
					.Text(FText::FromString(PendingNewWeaponName))
					.OnTextChanged_Lambda([this](const FText& Text)
					{
						PendingNewWeaponName = Text.ToString();
					})
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("WeaponFolderLabel", "内容目录"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 12.f)
			[
				SNew(SEditableTextBox)
					.Text(FText::FromString(PendingNewWeaponFolder))
					.OnTextChanged_Lambda([this](const FText& Text)
					{
						PendingNewWeaponFolder = Text.ToString();
					})
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
					.Text(LOCTEXT(
						"NewWeaponCreates",
						"将创建武器定义、普通攻击数据、被动反应数据和武器显示信息资产；不会修改任何地图、材质或渲染资产。"))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
						.Text(LOCTEXT("CancelCreate", "取消"))
						.OnClicked_Lambda([Dialog]()
						{
							Dialog->RequestDestroyWindow();
							return FReply::Handled();
						})
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
						.Text(LOCTEXT("ConfirmCreate", "创建"))
						.OnClicked_Lambda([this, Dialog]() { return ConfirmCreateWeapon(Dialog); })
				]
			]
		]);

	FSlateApplication::Get().AddModalWindow(Dialog, FSlateApplication::Get().GetActiveTopLevelWindow());
	return FReply::Handled();
}

FReply SWeaponManagerWidget::ConfirmCreateWeapon(TSharedPtr<SWindow> Dialog)
{
	FString CleanName = ObjectTools::SanitizeObjectName(PendingNewWeaponName);
	if (CleanName.StartsWith(TEXT("DA_WPN_")))
	{
		CleanName.RightChopInline(7);
	}
	FString Folder = NormalizeGameFolder(PendingNewWeaponFolder);
	if (Folder == TEXT("/Game/Code/Weapon/NewWeapon") && CleanName != TEXT("NewWeapon"))
	{
		Folder = TEXT("/Game/Code/Weapon/") + CleanName;
	}
	const bool bIsGameFolder = Folder == TEXT("/Game") || Folder.StartsWith(TEXT("/Game/"));
	if (CleanName.IsEmpty() || !bIsGameFolder)
	{
		SetStatus(LOCTEXT("InvalidWeaponCreate", "武器名称为空，或目标目录不在 /Game 下。"), true);
		return FReply::Handled();
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	auto MakeUnique = [&AssetTools, &Folder](const FString& BaseName, FString& OutPackage, FString& OutAsset)
	{
		AssetTools.CreateUniqueAssetName(Folder / BaseName, TEXT(""), OutPackage, OutAsset);
	};

	FString WeaponPackage;
	FString WeaponAssetName;
	FString AttackPackage;
	FString AttackAssetName;
	FString PassivePackage;
	FString PassiveAssetName;
	FString InfoPackage;
	FString InfoAssetName;
	MakeUnique(TEXT("DA_WPN_") + CleanName, WeaponPackage, WeaponAssetName);
	MakeUnique(TEXT("DA_Attack_") + CleanName, AttackPackage, AttackAssetName);
	MakeUnique(TEXT("DA_Passive_") + CleanName, PassivePackage, PassiveAssetName);
	MakeUnique(TEXT("DA_WPN_Info_") + CleanName, InfoPackage, InfoAssetName);

	TArray<UPackage*> Packages;
	UWeaponDefinition* Weapon = Cast<UWeaponDefinition>(
		CreateAsset(UWeaponDefinition::StaticClass(), WeaponPackage, WeaponAssetName, Packages));
	UWeaponAttackAbilityMontageData* AttackData = Cast<UWeaponAttackAbilityMontageData>(
		CreateAsset(UWeaponAttackAbilityMontageData::StaticClass(), AttackPackage, AttackAssetName, Packages));
	UWeaponPassiveAbilityMontageData* PassiveData = Cast<UWeaponPassiveAbilityMontageData>(
		CreateAsset(UWeaponPassiveAbilityMontageData::StaticClass(), PassivePackage, PassiveAssetName, Packages));
	UWeaponInfoDA* WeaponInfo = Cast<UWeaponInfoDA>(
		CreateAsset(UWeaponInfoDA::StaticClass(), InfoPackage, InfoAssetName, Packages));
	if (!Weapon || !AttackData || !PassiveData || !WeaponInfo)
	{
		SetStatus(LOCTEXT("CreateWeaponFailed", "一个或多个武器资产创建失败。"), true);
		return FReply::Handled();
	}

	Weapon->AttackAbilityData = AttackData;
	Weapon->PassiveAbilityData = PassiveData;
	Weapon->WeaponInfo = WeaponInfo;
	WeaponInfo->WeaponName = FText::FromString(CleanName);
	Weapon->MarkPackageDirty();
	AttackData->MarkPackageDirty();
	PassiveData->MarkPackageDirty();
	WeaponInfo->MarkPackageDirty();

	const bool bSaved = SavePackages(Packages, false);
	if (Dialog.IsValid())
	{
		Dialog->RequestDestroyWindow();
	}

	RefreshWeapons(false);
	for (const FWeaponItemPtr& Item : AllWeapons)
	{
		if (Item.IsValid() && Item->Weapon.Get() == Weapon)
		{
			SelectedWeaponItem = Item;
			if (WeaponListView.IsValid())
			{
				WeaponListView->SetSelection(Item);
			}
			break;
		}
	}
	RebuildCenterPanel();
	RefreshDetailsPanel();
	SetStatus(bSaved
		? FText::Format(LOCTEXT("WeaponCreatedStatus", "已创建 {0} 及其配套数据资产。"), FText::FromString(WeaponAssetName))
		: LOCTEXT("WeaponCreatedUnsaved", "武器资产已创建，但保存失败或被取消。"), !bSaved);
	return FReply::Handled();
}

UObject* SWeaponManagerWidget::CreateAsset(
	UClass* AssetClass,
	const FString& PackageName,
	const FString& AssetName,
	TArray<UPackage*>& OutPackages) const
{
	if (!AssetClass || !FPackageName::IsValidLongPackageName(PackageName))
	{
		return nullptr;
	}
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return nullptr;
	}
	UObject* Asset = NewObject<UObject>(
		Package,
		AssetClass,
		*AssetName,
		RF_Public | RF_Standalone | RF_Transactional);
	if (!Asset)
	{
		return nullptr;
	}
	FAssetRegistryModule::AssetCreated(Asset);
	Asset->MarkPackageDirty();
	OutPackages.AddUnique(Package);
	return Asset;
}

bool SWeaponManagerWidget::SavePackages(const TArray<UPackage*>& Packages, bool bPromptForCheckout) const
{
	TArray<UPackage*> ValidPackages;
	for (UPackage* Package : Packages)
	{
		if (Package)
		{
			ValidPackages.AddUnique(Package);
		}
	}
	if (ValidPackages.IsEmpty())
	{
		return true;
	}
	if (bPromptForCheckout)
	{
		return FEditorFileUtils::PromptForCheckoutAndSave(ValidPackages, false, false)
			!= FEditorFileUtils::PR_Failure;
	}
	return UEditorLoadingAndSavingUtils::SavePackages(ValidPackages, false);
}

FReply SWeaponManagerWidget::SaveManagedAssets()
{
	UWeaponDefinition* Weapon = GetSelectedWeapon();
	if (!Weapon)
	{
		return FReply::Handled();
	}
	TArray<UPackage*> Packages;
	auto AddAsset = [&Packages](UObject* Asset)
	{
		if (Asset)
		{
			Packages.AddUnique(Asset->GetOutermost());
		}
	};
	AddAsset(Weapon);
	AddAsset(Weapon->AttackAbilityData);
	AddAsset(Weapon->PassiveAbilityData);
	AddAsset(Weapon->WeaponInfo);
	for (UWeaponSkillDataAsset* Skill : Weapon->AvailableWeaponSkills)
	{
		AddAsset(Skill);
		AddAsset(Skill ? Skill->AbilityData.Get() : nullptr);
	}
	const bool bSaved = SavePackages(Packages, true);
	SetStatus(
		bSaved ? LOCTEXT("SaveSucceeded", "已保存当前武器及其配套数据资产。")
			: LOCTEXT("SaveFailed", "签出/保存失败或被取消。"),
		!bSaved);
	RefreshWeapons(true);
	RebuildCenterPanel();
	RefreshDetailsPanel();
	return FReply::Handled();
}

FReply SWeaponManagerWidget::OpenAsset(UObject* Asset) const
{
	if (GEditor && Asset)
	{
		if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			Subsystem->OpenEditorForAsset(Asset);
		}
	}
	return FReply::Handled();
}

FReply SWeaponManagerWidget::SyncAsset(UObject* Asset) const
{
	if (GEditor && Asset)
	{
		TArray<UObject*> ObjectsToSync;
		ObjectsToSync.Add(Asset);
		GEditor->SyncBrowserToObjects(ObjectsToSync);
	}
	return FReply::Handled();
}

FString SWeaponManagerWidget::GetWeaponFolder() const
{
	if (const UWeaponDefinition* Weapon = GetSelectedWeapon())
	{
		return FPackageName::GetLongPackagePath(Weapon->GetOutermost()->GetName());
	}
	return TEXT("/Game/Code/Weapon");
}

FString SWeaponManagerWidget::GetWeaponStem() const
{
	if (const UWeaponDefinition* Weapon = GetSelectedWeapon())
	{
		FString Stem = Weapon->GetName();
		Stem.RemoveFromStart(TEXT("DA_WPN_"));
		return ObjectTools::SanitizeObjectName(Stem);
	}
	return TEXT("Weapon");
}

FText SWeaponManagerWidget::GetSelectedWeaponTitle() const
{
	const UWeaponDefinition* Weapon = GetSelectedWeapon();
	if (!Weapon)
	{
		return LOCTEXT("NoWeaponSelected", "尚未选择武器");
	}
	const FText DisplayName = Weapon->WeaponInfo && !Weapon->WeaponInfo->WeaponName.IsEmpty()
		? Weapon->WeaponInfo->WeaponName
		: FText::FromString(Weapon->GetName());
	return FText::Format(
		LOCTEXT("SelectedWeaponTitleFmt", "{0}  |  {1}"),
		DisplayName,
		StaticEnum<EWeaponType>()->GetDisplayNameTextByValue(static_cast<int64>(Weapon->WeaponType)));
}

int32 SWeaponManagerWidget::CountWeaponIssues(const UWeaponDefinition* Weapon, int32& OutWarnings) const
{
	OutWarnings = 0;
	if (!Weapon)
	{
		return 1;
	}
	int32 Errors = 0;
	if (!Weapon->AttackAbilityData)
	{
		++OutWarnings;
	}
	if (!Weapon->PassiveAbilityData)
	{
		++OutWarnings;
	}
	if (!Weapon->WeaponInfo)
	{
		++OutWarnings;
	}
	if (!Weapon->DisplayMesh)
	{
		// The data asset remains usable for runtime-only definitions such as
		// unarmed combat, but cannot be previewed by a placed WeaponSpawner.
		++OutWarnings;
	}
	if (Weapon->DefaultWeaponSkill && !Weapon->AvailableWeaponSkills.Contains(Weapon->DefaultWeaponSkill))
	{
		++Errors;
	}
	TSet<FGameplayTag> SeenTags;
	for (const UWeaponSkillDataAsset* Skill : Weapon->AvailableWeaponSkills)
	{
		if (!Skill || !Skill->AbilityClass || !Skill->AbilityData)
		{
			++Errors;
			continue;
		}
		const FGameplayTag SkillTag = Skill->GetResolvedSkillTag();
		if (!SkillTag.IsValid() || SeenTags.Contains(SkillTag))
		{
			++Errors;
			continue;
		}
		SeenTags.Add(SkillTag);
		if (!Skill->SkillTag.IsValid())
		{
			++OutWarnings;
		}
		for (const FSkillTypePtr& SkillType : SkillTypes)
		{
			if (SkillType.IsValid()
				&& SkillType->SkillTag.MatchesTagExact(SkillTag)
				&& SkillType->DataAssetClass
				&& !Skill->IsA(SkillType->DataAssetClass))
			{
				// Existing pre-manager assets remain valid, but are flagged so
				// designers know they do not expose the GA-specific C++ fields.
				++OutWarnings;
				break;
			}
		}
	}
	if (!Weapon->DefaultWeaponSkill && !Weapon->AvailableWeaponSkills.IsEmpty())
	{
		++OutWarnings;
	}
	return Errors;
}

FText SWeaponManagerWidget::GetWeaponValidationText(const UWeaponDefinition* Weapon) const
{
	int32 Warnings = 0;
	const int32 Errors = CountWeaponIssues(Weapon, Warnings);
	if (Errors > 0)
	{
		return FText::Format(LOCTEXT("WeaponErrors", "{0} 个错误，{1} 个警告"), FText::AsNumber(Errors), FText::AsNumber(Warnings));
	}
	if (Warnings > 0)
	{
		return FText::Format(LOCTEXT("WeaponWarnings", "{0} 个警告"), FText::AsNumber(Warnings));
	}
	return LOCTEXT("WeaponValid", "校验通过");
}

void SWeaponManagerWidget::RefreshStatus()
{
	const UWeaponDefinition* Weapon = GetSelectedWeapon();
	if (!Weapon)
	{
		SetStatus(FText::Format(LOCTEXT("WeaponCountStatus", "共找到 {0} 个武器。"), FText::AsNumber(AllWeapons.Num())));
		return;
	}
	SetStatus(FText::Format(
		LOCTEXT("SelectedValidationStatus", "{0}: {1}"),
		FText::FromString(Weapon->GetName()),
		GetWeaponValidationText(Weapon)));
}

void SWeaponManagerWidget::SetStatus(const FText& InStatus, bool bIsError)
{
	StatusText = InStatus;
	bStatusIsError = bIsError;
}

FText SWeaponManagerWidget::GetStatusText() const
{
	return StatusText;
}

FSlateColor SWeaponManagerWidget::GetStatusColor() const
{
	return bStatusIsError
		? FSlateColor(FLinearColor(0.95f, 0.28f, 0.22f))
		: FSlateColor::UseForeground();
}

#undef LOCTEXT_NAMESPACE
