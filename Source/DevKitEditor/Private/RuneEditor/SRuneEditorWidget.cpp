#include "RuneEditor/SRuneEditorWidget.h"

#include "BuffFlow/Nodes/YogFlowNodes.h"
#include "BuffFlow/Nodes/BFNode_GetProjectileModule.h"
#include "BuffFlow/Nodes/BFNode_GetAuraModule.h"
#include "BuffFlow/Nodes/BFNode_CombatCardContext.h"
#include "BuffFlow/Nodes/BFNode_CompareFloat.h"
#include "BuffFlow/Nodes/BFNode_MathFloat.h"
#include "Data/RuneDataAsset.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Editor.h"
#include "FlowAsset.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "GraphEditor.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "Nodes/FlowNode.h"
#include "Nodes/Route/FlowNode_ExecutionSequence.h"
#include "PropertyEditorModule.h"
#include "RuneEditor/RuneEditorAuthoring.h"
#include "RuneEditor/RuneEditorValidation.h"
#include "ScopedTransaction.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Tools/DataEditorLibrary.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "SRuneEditorWidget"

namespace
{
	FString RuneTypeToString(ERuneType RuneType)
	{
		switch (RuneType)
		{
		case ERuneType::Buff: return TEXT("增益");
		case ERuneType::Debuff: return TEXT("减益");
		case ERuneType::None: return TEXT("无");
		}
		return TEXT("未知");
	}

	FString RarityToDisplayString(ERuneRarity Rarity)
	{
		switch (Rarity)
		{
		case ERuneRarity::Common: return TEXT("普通");
		case ERuneRarity::Rare: return TEXT("稀有");
		case ERuneRarity::Epic: return TEXT("史诗");
		case ERuneRarity::Legendary: return TEXT("传说");
		}
		return TEXT("未知");
	}

	FString TriggerToString(ERuneTriggerType TriggerType)
	{
		switch (TriggerType)
		{
		case ERuneTriggerType::Passive: return TEXT("被动");
		case ERuneTriggerType::OnAttackHit: return TEXT("攻击命中");
		case ERuneTriggerType::OnDash: return TEXT("冲刺");
		case ERuneTriggerType::OnKill: return TEXT("击杀");
		case ERuneTriggerType::OnCritHit: return TEXT("暴击");
		case ERuneTriggerType::OnDamageReceived: return TEXT("受到伤害");
		}
		return TEXT("未知");
	}

	FText TuningValueSourceToText(ERuneTuningValueSource Source)
	{
		switch (Source)
		{
		case ERuneTuningValueSource::Literal: return LOCTEXT("TuningSourceLiteral", "具体值");
		case ERuneTuningValueSource::Formula: return LOCTEXT("TuningSourceFormula", "公式");
		case ERuneTuningValueSource::MMC: return LOCTEXT("TuningSourceMMC", "MMC");
		case ERuneTuningValueSource::Context: return LOCTEXT("TuningSourceContext", "上下文");
		}
		return LOCTEXT("TuningSourceUnknown", "未知");
	}

	ERuneTuningValueSource GetNextTuningValueSource(ERuneTuningValueSource Source)
	{
		switch (Source)
		{
		case ERuneTuningValueSource::Literal: return ERuneTuningValueSource::Formula;
		case ERuneTuningValueSource::Formula: return ERuneTuningValueSource::MMC;
		case ERuneTuningValueSource::MMC: return ERuneTuningValueSource::Context;
		case ERuneTuningValueSource::Context: return ERuneTuningValueSource::Literal;
		}
		return ERuneTuningValueSource::Literal;
	}

	ERuneTuningValueSource ValueSourceFromString(const FString& S)
	{
		if (S == TEXT("具体值")) return ERuneTuningValueSource::Literal;
		if (S == TEXT("公式"))   return ERuneTuningValueSource::Formula;
		if (S == TEXT("MMC"))    return ERuneTuningValueSource::MMC;
		if (S == TEXT("上下文")) return ERuneTuningValueSource::Context;
		return ERuneTuningValueSource::Literal;
	}

	// ── 数值表预设 ─────────────────────────────────────────────────────────
	struct FTuningPresetRow
	{
		FName  Key;
		FString DisplayName;
		float  Value         = 0.f;
		FName  Category;
		FString Description;
		ERuneComboBonusMode   ComboMode      = ERuneComboBonusMode::None;
		float  BonusPerStack = 0.f;
		float  MaxBonus      = 0.f;
		ERuneTuningRoundMode  RoundMode      = ERuneTuningRoundMode::None;

		FRuneTuningScalar ToScalar() const
		{
			FRuneTuningScalar S;
			S.Key         = Key;
			S.DisplayName = FText::FromString(DisplayName);
			S.Value       = Value;
			S.Category    = Category;
			S.Description = FText::FromString(Description);
			S.ValueSource = ERuneTuningValueSource::Literal;
			S.ComboBonus.Mode          = ComboMode;
			S.ComboBonus.BonusPerStack = BonusPerStack;
			S.ComboBonus.MaxBonus      = MaxBonus;
			S.ComboBonus.RoundMode     = RoundMode;
			return S;
		}
	};

	struct FTuningPresetGroup
	{
		FString               Name;
		TArray<FTuningPresetRow> Rows;
	};

	const TArray<FTuningPresetGroup>& GetTuningPresets()
	{
		static TArray<FTuningPresetGroup> Presets;
		if (!Presets.IsEmpty()) { return Presets; }

		Presets = {
			{
				TEXT("攻击类"),
				{
					{ "Attack.Damage", TEXT("攻击伤害"), 15.f, "Damage", TEXT("攻击基础伤害；连招乘算，第N段 = 15×(1+0.1×(N-1))，封顶1.5倍"),
					  ERuneComboBonusMode::Multiply, 0.1f, 0.5f },
				}
			},
			{
				TEXT("燃烧类"),
				{
					{ "Burn.Damage", TEXT("燃烧伤害/周期"), 20.f, "Damage", TEXT("燃烧DoT每次触发的伤害量；连招乘算，封顶+60%"),
					  ERuneComboBonusMode::Multiply, 0.15f, 0.60f },
					{ "Burn.Duration", TEXT("燃烧持续时间"), 3.f, "Duration", TEXT("燃烧效果持续秒数") },
				}
			},
			{
				TEXT("中毒类"),
				{
					{ "Poison.Stack", TEXT("中毒层数"), 3.f, "Stack", TEXT("附加的中毒层数；连招加算，第N段 = 3+(N-1)，封顶+4层"),
					  ERuneComboBonusMode::Add, 1.f, 4.f, ERuneTuningRoundMode::Floor },
					{ "Poison.Duration", TEXT("中毒持续时间"), 6.f, "Duration", TEXT("每层中毒持续秒数") },
				}
			},
			{
				TEXT("月光类"),
				{
					{ "Moonlight.ProjectileCount", TEXT("月光弹数"), 1.f, "Projectile", TEXT("发射的月光投射物数量；连招加算，第N段 = 1+(N-1)，封顶+3发"),
					  ERuneComboBonusMode::Add, 1.f, 3.f, ERuneTuningRoundMode::Floor },
					{ "Moonlight.ProjectileSpeed", TEXT("月光弹速"), 2000.f, "Projectile", TEXT("月光投射物飞行速度（cm/s）") },
				}
			},
			{
				TEXT("终结技类"),
				{
					{ "Finisher.Damage",    TEXT("终结技伤害"),    80.f,  "Damage", TEXT("终结技基础伤害，建议不受连招缩放") },
					{ "Finisher.AOERadius", TEXT("终结技范围半径"), 300.f, "Radius", TEXT("终结技范围攻击半径（cm）") },
				}
			},
		};
		return Presets;
	}
	// ── Key 名称预定义枚举 ────────────────────────────────────────────────
	struct FPredefinedKeyGroup
	{
		FString          SectionName;
		TArray<FName>    Keys;
	};

	const TArray<FPredefinedKeyGroup>& GetPredefinedTuningKeyGroups()
	{
		static TArray<FPredefinedKeyGroup> Groups;
		if (!Groups.IsEmpty()) { return Groups; }
		Groups = {
			{ TEXT("攻击"),  {
				"Attack.Damage.01", "Attack.Damage.02", "Attack.Damage.03",
			}},
			{ TEXT("燃烧"),  {
				"Burn.Damage.01",   "Burn.Damage.02",   "Burn.Damage.03",
				"Burn.Duration.01", "Burn.Duration.02",
			}},
			{ TEXT("中毒"),  {
				"Poison.Stack.01",    "Poison.Stack.02",
				"Poison.Duration.01", "Poison.Duration.02",
			}},
			{ TEXT("月光"),  {
				"Moonlight.ProjectileCount.01", "Moonlight.ProjectileCount.02",
				"Moonlight.ProjectileSpeed.01",
			}},
			{ TEXT("终结技"), {
				"Finisher.Damage.01",    "Finisher.Damage.02",
				"Finisher.AOERadius.01",
			}},
		};
		return Groups;
	}
	// ──────────────────────────────────────────────────────────────────────

	ERuneType RuneTypeFromString(const FString& S)
	{
		if (S == TEXT("增益")) return ERuneType::Buff;
		if (S == TEXT("减益")) return ERuneType::Debuff;
		return ERuneType::None;
	}

	ERuneRarity RarityFromDisplayString(const FString& S)
	{
		if (S == TEXT("稀有"))  return ERuneRarity::Rare;
		if (S == TEXT("史诗"))  return ERuneRarity::Epic;
		if (S == TEXT("传说"))  return ERuneRarity::Legendary;
		return ERuneRarity::Common;
	}

	ERuneTriggerType TriggerFromString(const FString& S)
	{
		if (S == TEXT("攻击命中"))   return ERuneTriggerType::OnAttackHit;
		if (S == TEXT("冲刺"))       return ERuneTriggerType::OnDash;
		if (S == TEXT("击杀"))       return ERuneTriggerType::OnKill;
		if (S == TEXT("暴击"))       return ERuneTriggerType::OnCritHit;
		if (S == TEXT("受到伤害"))   return ERuneTriggerType::OnDamageReceived;
		return ERuneTriggerType::Passive;
	}

	FRuneTuningScalar* GetMutableTuningScalar(URuneDataAsset* Rune, int32 Index)
	{
		if (!Rune || !Rune->RuneInfo.RuneConfig.TuningScalars.IsValidIndex(Index))
		{
			return nullptr;
		}
		return &Rune->RuneInfo.RuneConfig.TuningScalars[Index];
	}

	const FRuneTuningScalar* GetTuningScalar(const URuneDataAsset* Rune, int32 Index)
	{
		if (!Rune || !Rune->RuneInfo.RuneConfig.TuningScalars.IsValidIndex(Index))
		{
			return nullptr;
		}
		return &Rune->RuneInfo.RuneConfig.TuningScalars[Index];
	}

	TSharedRef<SWidget> MakeDetailLine(const FText& Label, const TAttribute<FText>& Value)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 3.f)
			[
				SNew(STextBlock)
				.Text(Label)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(Value)
				.AutoWrapText(true)
			];
	}

	TSharedRef<SWidget> MakeSection(const FText& Title, const TSharedRef<SWidget>& Content)
	{
		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			.Padding(10.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(Title)
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					Content
				]
			];
	}

	FGameplayTag RequestOptionalTag(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName), false);
	}

	FString CombatCardTypeToString(ECombatCardType CardType)
	{
		switch (CardType)
		{
		case ECombatCardType::Attack:   return TEXT("Attack");
		case ECombatCardType::Link:     return TEXT("连携");
		case ECombatCardType::Finisher: return TEXT("终结技");
		case ECombatCardType::Passive:  return TEXT("被动");
		case ECombatCardType::Normal:   return TEXT("普通");
		}
		return TEXT("未知");
	}

	ECombatCardType CombatCardTypeFromString(const FString& S)
	{
		if (S == TEXT("连携"))   return ECombatCardType::Link;
		if (S == TEXT("终结技")) return ECombatCardType::Finisher;
		if (S == TEXT("被动"))   return ECombatCardType::Passive;
		if (S == TEXT("普通"))   return ECombatCardType::Normal;
		return ECombatCardType::Attack;
	}

	FString CardRequiredActionToString(ECardRequiredAction Action)
	{
		switch (Action)
		{
		case ECardRequiredAction::Light: return TEXT("轻击");
		case ECardRequiredAction::Heavy: return TEXT("重击");
		case ECardRequiredAction::Any:   return TEXT("任意");
		}
		return TEXT("未知");
	}

	ECardRequiredAction CardRequiredActionFromString(const FString& S)
	{
		if (S == TEXT("重击")) return ECardRequiredAction::Heavy;
		if (S == TEXT("任意")) return ECardRequiredAction::Any;
		return ECardRequiredAction::Light;
	}

	FString CardTriggerTimingToString(ECombatCardTriggerTiming Timing)
	{
		switch (Timing)
		{
		case ECombatCardTriggerTiming::OnHit:    return TEXT("命中时 (OnHit)");
		case ECombatCardTriggerTiming::OnCommit: return TEXT("提交时 (OnCommit)");
		}
		return TEXT("未知");
	}

	ECombatCardTriggerTiming CardTriggerTimingFromString(const FString& S)
	{
		if (S == TEXT("提交时 (OnCommit)")) return ECombatCardTriggerTiming::OnCommit;
		return ECombatCardTriggerTiming::OnHit;
	}

	FString ComboBonusModeToString(ERuneComboBonusMode Mode)
	{
		switch (Mode)
		{
		case ERuneComboBonusMode::Add:      return TEXT("加算");
		case ERuneComboBonusMode::Multiply: return TEXT("乘算");
		default:                            return TEXT("无");
		}
	}

	ERuneComboBonusMode ComboBonusModeFromString(const FString& S)
	{
		if (S == TEXT("加算"))  return ERuneComboBonusMode::Add;
		if (S == TEXT("乘算"))  return ERuneComboBonusMode::Multiply;
		return ERuneComboBonusMode::None;
	}

	FString RoundModeToString(ERuneTuningRoundMode Mode)
	{
		switch (Mode)
		{
		case ERuneTuningRoundMode::Floor: return TEXT("Floor");
		case ERuneTuningRoundMode::Round: return TEXT("Round");
		case ERuneTuningRoundMode::Ceil:  return TEXT("Ceil");
		default:                          return TEXT("—");
		}
	}

	ERuneTuningRoundMode RoundModeFromString(const FString& S)
	{
		if (S == TEXT("Floor")) return ERuneTuningRoundMode::Floor;
		if (S == TEXT("Round")) return ERuneTuningRoundMode::Round;
		if (S == TEXT("Ceil"))  return ERuneTuningRoundMode::Ceil;
		return ERuneTuningRoundMode::None;
	}

	FString LibraryCategoryToString(const FGameplayTagContainer& Tags)
	{
		TArray<FString> Labels;
		auto AddIfHasTag = [&Labels, &Tags](const TCHAR* TagName, const TCHAR* Label)
		{
			const FGameplayTag Tag = RequestOptionalTag(TagName);
			if (Tag.IsValid() && Tags.HasTagExact(Tag))
			{
				Labels.Add(Label);
			}
		};

		AddIfHasTag(TEXT("Rune.Library.Base"), TEXT("基础节点"));
		AddIfHasTag(TEXT("Rune.Library.Enemy"), TEXT("敌人"));
		AddIfHasTag(TEXT("Rune.Library.Level"), TEXT("关卡"));
		AddIfHasTag(TEXT("Rune.Library.Finisher"), TEXT("终结技"));
		AddIfHasTag(TEXT("Rune.Library.ComboCard"), TEXT("连携卡牌"));

		return Labels.Num() > 0 ? FString::Join(Labels, TEXT(" | ")) : TEXT("未分类");
	}
}

void SRuneEditorWidget::Construct(const FArguments& InArgs)
{
	RuneTypeOptions    = { MakeShared<FString>(TEXT("增益")), MakeShared<FString>(TEXT("减益")), MakeShared<FString>(TEXT("无")) };
	RarityOptions      = { MakeShared<FString>(TEXT("普通")), MakeShared<FString>(TEXT("稀有")), MakeShared<FString>(TEXT("史诗")), MakeShared<FString>(TEXT("传说")) };
	TriggerTypeOptions = { MakeShared<FString>(TEXT("被动")), MakeShared<FString>(TEXT("攻击命中")), MakeShared<FString>(TEXT("冲刺")), MakeShared<FString>(TEXT("击杀")), MakeShared<FString>(TEXT("暴击")), MakeShared<FString>(TEXT("受到伤害")) };
	CardTypeOptions       = { MakeShared<FString>(TEXT("Attack")), MakeShared<FString>(TEXT("连携")), MakeShared<FString>(TEXT("终结技")), MakeShared<FString>(TEXT("被动")), MakeShared<FString>(TEXT("普通")) };
	RequiredActionOptions = { MakeShared<FString>(TEXT("轻击")), MakeShared<FString>(TEXT("重击")), MakeShared<FString>(TEXT("任意")) };
	TriggerTimingOptions  = { MakeShared<FString>(TEXT("命中时 (OnHit)")), MakeShared<FString>(TEXT("提交时 (OnCommit)")) };

	ComboBonusModeOptions = { MakeShared<FString>(TEXT("无")), MakeShared<FString>(TEXT("加算")), MakeShared<FString>(TEXT("乘算")) };
	RoundModeOptions      = { MakeShared<FString>(TEXT("—")), MakeShared<FString>(TEXT("Floor")), MakeShared<FString>(TEXT("Round")), MakeShared<FString>(TEXT("Ceil")) };
	ValueSourceOptions    = { MakeShared<FString>(TEXT("具体值")), MakeShared<FString>(TEXT("公式")), MakeShared<FString>(TEXT("MMC")), MakeShared<FString>(TEXT("上下文")) };

	for (const FTuningPresetGroup& Group : GetTuningPresets())
	{
		TuningPresetGroupNames.Add(MakeShared<FString>(Group.Name));
	}

	RefreshFlowAssetOptions();
	RefreshData(LOCTEXT("InitialStatus", "符文流程编辑器已就绪。"));
	RunFeedbackText = LOCTEXT("RunFeedbackInitial", "本次编辑器会话尚未运行符文。");

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildToolbar()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot()
			.Value(0.20f)
			[
				BuildResourceManagerPanel()
			]
			+ SSplitter::Slot()
			.Value(0.58f)
			[
				BuildCenterPanel()
			]
			+ SSplitter::Slot()
			.Value(0.22f)
			[
				BuildDetailsPanel()
			]
		]
	];

	RebuildGraphEditor();
	SyncNodeInspector();
}

TSharedRef<SWidget> SRuneEditorWidget::BuildToolbar()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Title", "符文编辑器"))
				.Font(FAppStyle::GetFontStyle(TEXT("HeadingMedium")))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Scope", "Yog 符文流程"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SRuneEditorWidget::GetStatusText)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("RunRune", "运行符文"))
				.IsEnabled_Lambda([this]()
				{
					return GetSelectedRune() != nullptr;
				})
				.OnClicked(this, &SRuneEditorWidget::OnRunRuneClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenRune", "打开符文"))
				.IsEnabled_Lambda([this]()
				{
					return GetSelectedRune() != nullptr;
				})
				.OnClicked(this, &SRuneEditorWidget::OnOpenRuneClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("OpenFlow", "打开流程"))
				.IsEnabled_Lambda([this]()
				{
					return GetSelectedFlowAsset() != nullptr;
				})
				.OnClicked(this, &SRuneEditorWidget::OnOpenFlowClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.f, 0.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "刷新"))
				.OnClicked(this, &SRuneEditorWidget::OnRefreshClicked)
			]
		];
}

TSharedRef<SWidget> SRuneEditorWidget::BuildResourceManagerPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(8.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ResourceManagerTitle", "资源管理"))
				.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FRuneEditorAuthoring::GetDefaultAssetRoot()))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				MakeSection(
					LOCTEXT("CreateRuneSection", "新建符文"),
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SAssignNew(NewRuneNameTextBox, SEditableTextBox)
						.HintText(LOCTEXT("NewRuneNameHint", "符文显示名"))
						.Text(LOCTEXT("DefaultNewRuneName", "新符文"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SAssignNew(NewRuneTagTextBox, SEditableTextBox)
						.HintText(LOCTEXT("NewRuneTagHint", "Rune.ID.NewRune"))
						.Text(LOCTEXT("DefaultNewRuneTag", "Rune.ID.NewRune"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SAssignNew(NewRuneFolderTextBox, SEditableTextBox)
						.HintText(LOCTEXT("NewRuneFolderHint", "子文件夹（留空=默认）"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.f, 0.f, 6.f, 0.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("CreateTypeLabel", "类型"))
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(CreateRuneTypeCombo, FStringCombo)
							.OptionsSource(&RuneTypeOptions)
							.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Value, ESelectInfo::Type)
							{
								if (Value.IsValid()) CreateRuneType = RuneTypeFromString(*Value);
							})
							.OnGenerateWidget_Lambda([](TSharedPtr<FString> Value)
							{
								return SNew(STextBlock).Text(FText::FromString(*Value)).Margin(FMargin(4.f, 2.f));
							})
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return FText::FromString(RuneTypeToString(CreateRuneType)); })
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(12.f, 0.f, 6.f, 0.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("CreateRarityLabel", "品质"))
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(CreateRarityCombo, FStringCombo)
							.OptionsSource(&RarityOptions)
							.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Value, ESelectInfo::Type)
							{
								if (Value.IsValid()) CreateRarity = RarityFromDisplayString(*Value);
							})
							.OnGenerateWidget_Lambda([](TSharedPtr<FString> Value)
							{
								return SNew(STextBlock).Text(FText::FromString(*Value)).Margin(FMargin(4.f, 2.f));
							})
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return FText::FromString(RarityToDisplayString(CreateRarity)); })
							]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0.f, 0.f, 6.f, 0.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("CreateTriggerLabel", "触发"))
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(CreateTriggerTypeCombo, FStringCombo)
							.OptionsSource(&TriggerTypeOptions)
							.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Value, ESelectInfo::Type)
							{
								if (Value.IsValid()) CreateTriggerType = TriggerFromString(*Value);
							})
							.OnGenerateWidget_Lambda([](TSharedPtr<FString> Value)
							{
								return SNew(STextBlock).Text(FText::FromString(*Value)).Margin(FMargin(4.f, 2.f));
							})
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return FText::FromString(TriggerToString(CreateTriggerType)); })
							]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("NewRuneButton", "新建符文"))
						.OnClicked(this, &SRuneEditorWidget::OnCreateRuneClicked)
					])
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				MakeSection(
					LOCTEXT("ResourceActionsSection", "资源操作"),
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SAssignNew(ResourceRenameTextBox, SEditableTextBox)
						.HintText(LOCTEXT("ResourceRenameHint", "新的资源名"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SUniformGridPanel)
						.SlotPadding(FMargin(2.f))
						+ SUniformGridPanel::Slot(0, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("CopyResource", "复制"))
							.IsEnabled_Lambda([this]()
							{
								return GetSelectedResource() != nullptr;
							})
							.OnClicked(this, &SRuneEditorWidget::OnCopyAssetClicked)
						]
						+ SUniformGridPanel::Slot(1, 0)
						[
							SNew(SButton)
							.Text(LOCTEXT("PasteResource", "粘贴"))
							.IsEnabled_Lambda([this]()
							{
								return CopiedResource.IsValid();
							})
							.OnClicked(this, &SRuneEditorWidget::OnPasteAssetClicked)
						]
						+ SUniformGridPanel::Slot(0, 1)
						[
							SNew(SButton)
							.Text(LOCTEXT("RenameResource", "重命名"))
							.IsEnabled_Lambda([this]()
							{
								return GetSelectedResource() != nullptr;
							})
							.OnClicked(this, &SRuneEditorWidget::OnRenameAssetClicked)
						]
						+ SUniformGridPanel::Slot(1, 1)
						[
							SNew(SButton)
							.Text(LOCTEXT("DeleteResource", "删除"))
							.IsEnabled_Lambda([this]()
							{
								return GetSelectedResource() != nullptr;
							})
							.OnClicked(this, &SRuneEditorWidget::OnDeleteAssetClicked)
						]
						+ SUniformGridPanel::Slot(0, 2)
						[
							SNew(SButton)
							.Text(LOCTEXT("LocateResource", "定位"))
							.IsEnabled_Lambda([this]()
							{
								return GetSelectedResource() != nullptr;
							})
							.OnClicked(this, &SRuneEditorWidget::OnLocateAssetClicked)
						]
					])
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				BuildRuneResourceListPanel()
			]
		];
}

TSharedRef<SWidget> SRuneEditorWidget::BuildRuneResourceListPanel()
{
	return MakeSection(
		LOCTEXT("AuthoredRunesSection", "符文资源"),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 6.f)
		[
			SAssignNew(SearchTextBox, SEditableTextBox)
			.HintText(LOCTEXT("SearchRuneHint", "搜索名称 / Tag / 分类..."))
			.OnTextChanged(this, &SRuneEditorWidget::OnSearchTextChanged)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SWrapBox)
			.UseAllottedSize(true)
			+ SWrapBox::Slot()
			.Padding(0.f, 0.f, 6.f, 6.f)
			[
				BuildResourceFilterButton(LOCTEXT("ResourceFilterAll", "全部"), EResourceFilter::All)
			]
			+ SWrapBox::Slot()
			.Padding(0.f, 0.f, 6.f, 6.f)
			[
				BuildResourceFilterButton(LOCTEXT("ResourceFilterBase", "基础节点"), EResourceFilter::Base)
			]
			+ SWrapBox::Slot()
			.Padding(0.f, 0.f, 6.f, 6.f)
			[
				BuildResourceFilterButton(LOCTEXT("ResourceFilterEnemy", "敌人"), EResourceFilter::Enemy)
			]
			+ SWrapBox::Slot()
			.Padding(0.f, 0.f, 6.f, 6.f)
			[
				BuildResourceFilterButton(LOCTEXT("ResourceFilterLevel", "关卡"), EResourceFilter::Level)
			]
			+ SWrapBox::Slot()
			.Padding(0.f, 0.f, 6.f, 6.f)
			[
				BuildResourceFilterButton(LOCTEXT("ResourceFilterFinisher", "终结技"), EResourceFilter::Finisher)
			]
			+ SWrapBox::Slot()
			.Padding(0.f, 0.f, 6.f, 6.f)
			[
				BuildResourceFilterButton(LOCTEXT("ResourceFilterComboCard", "连携卡牌"), EResourceFilter::ComboCard)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SBox)
			.MinDesiredHeight(360.f)
			[
				SAssignNew(RuneListView, SListView<FRuneRowPtr>)
				.ListItemsSource(&RuneRows)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SRuneEditorWidget::GenerateRuneRow)
				.OnSelectionChanged(this, &SRuneEditorWidget::OnRuneSelectionChanged)
			]
		]);
}

TSharedRef<SWidget> SRuneEditorWidget::BuildCenterPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(8.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 10.f)
			[
				MakeSection(
					LOCTEXT("SelectedFlowSection", "当前符文流程"),
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 8.f)
					[
						MakeDetailLine(LOCTEXT("SelectedRuneLabel", "符文"), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SRuneEditorWidget::GetSelectedRuneNameText)))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						MakeDetailLine(LOCTEXT("SelectedFlowLabel", "流程资产"), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SRuneEditorWidget::GetSelectedFlowText)))
					])
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NativeGraphAuthoringHint", "在图表空白处右键，或从引脚拖线来添加节点；选中节点后可在右侧编辑属性。"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 6.f, 0.f)
					[
						BuildCenterTabButton(LOCTEXT("ValueTableCenterTab", "数值表"), ECenterPanelTab::ValueTable)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 6.f, 0.f)
					[
						BuildCenterTabButton(LOCTEXT("FlowGraphCenterTab", "流程图"), ECenterPanelTab::FlowGraph)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						BuildCenterTabButton(LOCTEXT("ComboRecipeCenterTab", "连携配方"), ECenterPanelTab::ComboRecipe)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(6.f, 0.f, 0.f, 0.f)
					[
						BuildCenterTabButton(LOCTEXT("ModulesCenterTab", "模块配置"), ECenterPanelTab::Modules)
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				[
					SAssignNew(CenterPanelSwitcher, SWidgetSwitcher)
					.WidgetIndex(static_cast<int32>(ActiveCenterTab))
					+ SWidgetSwitcher::Slot()
					[
						BuildValueTablePanel()
					]
					+ SWidgetSwitcher::Slot()
					[
						SNew(SSplitter)
						.Orientation(Orient_Vertical)
						+ SSplitter::Slot()
						.Value(0.84f)
						[
							BuildGraphEditorPanel()
						]
						+ SSplitter::Slot()
						.Value(0.16f)
						[
							BuildBottomDiagnosticsPanel()
						]
					]
					+ SWidgetSwitcher::Slot()
					[
						BuildComboRecipePanel()
					]
					+ SWidgetSwitcher::Slot()
					[
						BuildModulesPanel()
					]
				]
			]
		];
}

TSharedRef<SWidget> SRuneEditorWidget::BuildCenterTabButton(const FText& Label, ECenterPanelTab Tab)
{
	return SNew(SButton)
		.Text_Lambda([this, Label, Tab]()
		{
			return ActiveCenterTab == Tab
				? FText::Format(LOCTEXT("ActiveCenterTabLabel", "● {0}"), Label)
				: Label;
		})
		.OnClicked(this, &SRuneEditorWidget::OnCenterTabSelected, Tab);
}

TSharedRef<SWidget> SRuneEditorWidget::BuildGraphEditorPanel()
{
	return MakeSection(
		LOCTEXT("BlueprintStyleFlowGraphSection", "流程图"),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BlueprintStyleFlowGraphHint", "右键空白区域搜索节点，或从节点引脚拖线创建连接。"))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SAssignNew(GraphEditorContainer, SBox)
			.MinDesiredHeight(620.f)
		]);
}

TSharedRef<SWidget> SRuneEditorWidget::BuildValueTablePanel()
{
	return MakeSection(
		LOCTEXT("RuneValueTableSection", "数值表"),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("RuneValueTableHint", "在这里统一配置伤害、持续时间、半径、概率、层数等数值；流程图节点只需要引用对应 Key。"))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("AddTuningRow", "新增数值"))
				.IsEnabled_Lambda([this]()
				{
					return GetSelectedRune() != nullptr;
				})
				.OnClicked(this, &SRuneEditorWidget::OnAddTuningRowClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SAssignNew(TuningPresetCombo, FStringCombo)
				.OptionsSource(&TuningPresetGroupNames)
				.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
				.ToolTipText(LOCTEXT("InsertPresetTip", "选择符文类型，批量插入对应的预设数值行（含连招加成配置）"))
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type SelectType)
				{
					if (SelectType == ESelectInfo::Direct || !Item.IsValid()) { return; }
					OnInsertTuningPresetClicked(*Item);
					TuningPresetCombo->ClearSelection();
				})
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(*Item)).Margin(FMargin(6.f, 3.f));
				})
				[
					SNew(STextBlock).Text(LOCTEXT("InsertPresetLabel", "插入模板 ▼"))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ExportTuning", "导出 CSV"))
				.ToolTipText(LOCTEXT("ExportTuningTip", "将当前数值表复制为 CSV 到剪贴板"))
				.IsEnabled_Lambda([this]()
				{
					return GetSelectedRune() != nullptr;
				})
				.OnClicked(this, &SRuneEditorWidget::OnExportTuningClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("ImportTuning", "导入 CSV"))
				.ToolTipText(LOCTEXT("ImportTuningTip", "从剪贴板读取 CSV 覆盖当前数值表（格式与导出一致）"))
				.IsEnabled_Lambda([this]()
				{
					return GetSelectedRune() != nullptr;
				})
				.OnClicked(this, &SRuneEditorWidget::OnImportTuningClicked)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 4.f)
		[
			BuildTuningCategoryFilterBar()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SAssignNew(TuningListView, SListView<FTuningRowPtr>)
			.ListItemsSource(&TuningRows)
			.SelectionMode(ESelectionMode::None)
			.OnGenerateRow(this, &SRuneEditorWidget::BuildTuningRow)
			.HeaderRow(
				SNew(SHeaderRow)
				+ SHeaderRow::Column(TEXT("Key")).DefaultLabel(LOCTEXT("TuningColumnKey", "Key")).FillWidth(1.8f)
				+ SHeaderRow::Column(TEXT("Name")).DefaultLabel(LOCTEXT("TuningColumnName", "显示名")).FillWidth(1.0f)
				+ SHeaderRow::Column(TEXT("Category")).DefaultLabel(LOCTEXT("TuningColumnCategory", "分类")).FillWidth(0.8f)
				+ SHeaderRow::Column(TEXT("Source")).DefaultLabel(LOCTEXT("TuningColumnSource", "数值方式")).FillWidth(0.7f)
				+ SHeaderRow::Column(TEXT("Value")).DefaultLabel(LOCTEXT("TuningColumnValue", "具体值")).FillWidth(0.65f)
				+ SHeaderRow::Column(TEXT("Formula")).DefaultLabel(LOCTEXT("TuningColumnFormula", "公式")).FillWidth(1.1f)
				+ SHeaderRow::Column(TEXT("Context")).DefaultLabel(LOCTEXT("TuningColumnContext", "上下文")).FillWidth(0.9f)
				+ SHeaderRow::Column(TEXT("Min")).DefaultLabel(LOCTEXT("TuningColumnMin", "最小值")).FillWidth(0.55f)
				+ SHeaderRow::Column(TEXT("Max")).DefaultLabel(LOCTEXT("TuningColumnMax", "最大值")).FillWidth(0.55f)
				+ SHeaderRow::Column(TEXT("ValueTag")).DefaultLabel(LOCTEXT("TuningColumnValueTag", "ValueTag")).FillWidth(0.9f)
				+ SHeaderRow::Column(TEXT("Unit")).DefaultLabel(LOCTEXT("TuningColumnUnit", "单位")).FillWidth(0.55f)
				+ SHeaderRow::Column(TEXT("Description")).DefaultLabel(LOCTEXT("TuningColumnDescription", "说明")).FillWidth(1.2f)
				+ SHeaderRow::Column(TEXT("ComboMode")).DefaultLabel(LOCTEXT("TuningColumnComboMode", "连招模式")).FillWidth(0.65f)
				+ SHeaderRow::Column(TEXT("BonusPerStack")).DefaultLabel(LOCTEXT("TuningColumnBonusPerStack", "每段奖励")).FillWidth(0.6f)
				+ SHeaderRow::Column(TEXT("MaxBonus")).DefaultLabel(LOCTEXT("TuningColumnMaxBonus", "奖励上限")).FillWidth(0.6f)
				+ SHeaderRow::Column(TEXT("RoundMode")).DefaultLabel(LOCTEXT("TuningColumnRoundMode", "取整")).FillWidth(0.55f)
				+ SHeaderRow::Column(TEXT("Actions")).DefaultLabel(LOCTEXT("TuningColumnActions", "操作")).FixedWidth(64.f))
		]);
}

TSharedRef<SWidget> SRuneEditorWidget::BuildBottomDiagnosticsPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.BorderBackgroundColor(FLinearColor(0.015f, 0.015f, 0.015f, 1.f))
		.Padding(8.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					BuildBottomTabButton(LOCTEXT("NodeLibraryTab", "节点库"), EBottomPanelTab::NodeLibrary)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					BuildBottomTabButton(LOCTEXT("ValidationTab", "校验"), EBottomPanelTab::Validation)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					BuildBottomTabButton(LOCTEXT("RunLogTab", "运行日志"), EBottomPanelTab::RunLog)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					BuildBottomTabButton(LOCTEXT("SelectedNodeTab", "选中节点"), EBottomPanelTab::SelectedNode)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SAssignNew(BottomPanelSwitcher, SWidgetSwitcher)
				.WidgetIndex(0)
				+ SWidgetSwitcher::Slot()
				[
					BuildNodeLibraryPanel()
				]
				+ SWidgetSwitcher::Slot()
				[
					BuildValidationPanel()
				]
				+ SWidgetSwitcher::Slot()
				[
					BuildRunLogPanel()
				]
				+ SWidgetSwitcher::Slot()
				[
					BuildSelectedNodePanel()
				]
			]
		];
}

TSharedRef<SWidget> SRuneEditorWidget::BuildBottomTabButton(const FText& Label, EBottomPanelTab Tab)
{
	return SNew(SButton)
		.Text_Lambda([this, Label, Tab]()
		{
			return ActiveBottomTab == Tab
				? FText::Format(LOCTEXT("ActiveBottomTabLabel", "● {0}"), Label)
				: Label;
		})
		.OnClicked(this, &SRuneEditorWidget::OnBottomTabSelected, Tab);
}

TSharedRef<SWidget> SRuneEditorWidget::BuildResourceFilterButton(const FText& Label, EResourceFilter Filter)
{
	return SNew(SButton)
		.Text_Lambda([this, Label, Filter]()
		{
			return ActiveResourceFilter == Filter
				? FText::Format(LOCTEXT("ActiveResourceFilterLabel", "● {0}"), Label)
				: Label;
		})
		.OnClicked(this, &SRuneEditorWidget::OnResourceFilterSelected, Filter);
}

TSharedRef<SWidget> SRuneEditorWidget::BuildLibraryCategoryToggle(const FText& Label, const FGameplayTag& CategoryTag)
{
	return SNew(SButton)
		.Text_Lambda([this, Label, CategoryTag]()
		{
			return IsRuneLibraryCategoryActive(CategoryTag)
				? FText::Format(LOCTEXT("ActiveLibraryCategoryLabel", "● {0}"), Label)
				: Label;
		})
		.IsEnabled_Lambda([this]()
		{
			return GetSelectedRune() != nullptr;
		})
		.OnClicked(this, &SRuneEditorWidget::OnLibraryCategoryToggled, CategoryTag);
}

TSharedRef<SWidget> SRuneEditorWidget::BuildNodeLibraryFilterButton(const FText& Label, ENodeLibraryFilter Filter)
{
	return SNew(SButton)
		.Text_Lambda([this, Label, Filter]()
		{
			return ActiveNodeLibraryFilter == Filter
				? FText::Format(LOCTEXT("ActiveNodeLibraryFilterLabel", "● {0}"), Label)
				: Label;
		})
		.OnClicked(this, &SRuneEditorWidget::OnNodeLibraryFilterSelected, Filter);
}

TSharedRef<SWidget> SRuneEditorWidget::BuildNodeLibraryPanel()
{
	TSharedRef<SWrapBox> FilterWrap = SNew(SWrapBox).UseAllottedSize(true);
	auto AddFilter = [this, &FilterWrap](const FText& Label, ENodeLibraryFilter Filter)
	{
		FilterWrap->AddSlot()
			.Padding(0.f, 0.f, 6.f, 6.f)
			[
				BuildNodeLibraryFilterButton(Label, Filter)
			];
	};

	AddFilter(LOCTEXT("NodeLibraryFilterAll", "全部"), ENodeLibraryFilter::All);
	AddFilter(LOCTEXT("NodeLibraryFilterSkill", "技能"), ENodeLibraryFilter::Skill);
	AddFilter(LOCTEXT("NodeLibraryFilterEffect", "效果节点"), ENodeLibraryFilter::Effect);
	AddFilter(LOCTEXT("NodeLibraryFilterTask", "任务节点"), ENodeLibraryFilter::Task);
	AddFilter(LOCTEXT("NodeLibraryFilterSpawn", "生成节点"), ENodeLibraryFilter::Spawn);
	AddFilter(LOCTEXT("NodeLibraryFilterCondition", "条件节点"), ENodeLibraryFilter::Condition);
	AddFilter(LOCTEXT("NodeLibraryFilterPresentation", "表现节点"), ENodeLibraryFilter::Presentation);
	AddFilter(LOCTEXT("NodeLibraryFilterLifecycle", "生命周期"), ENodeLibraryFilter::Lifecycle);
	AddFilter(LOCTEXT("NodeLibraryFilterPure", "数据节点"), ENodeLibraryFilter::Pure);

	TSharedRef<SWrapBox> NodeWrap = SNew(SWrapBox).UseAllottedSize(true);
	auto AddNode = [this, &NodeWrap](ENodeLibraryFilter Filter, UClass* NodeClass, const FText& DisplayName, const FText& Description)
	{
		NodeWrap->AddSlot()
			.Padding(0.f, 0.f, 10.f, 10.f)
			[
				BuildNodeLibraryItem(Filter, NodeClass, DisplayName, Description)
			];
	};

	AddNode(ENodeLibraryFilter::Skill, UYogFlowNode_SkillPass::StaticClass(), LOCTEXT("NodeSkillPassName", "流程控制"), LOCTEXT("NodeSkillPassDescription", "串联技能或符文流程，默认直接触发下一节点。"));
	AddNode(ENodeLibraryFilter::Skill, UFlowNode_ExecutionSequence::StaticClass(), LOCTEXT("NodeForkName", "分叉"), LOCTEXT("NodeForkDescription", "一个输入同时触发多个输出，适合从同一时机启动多条独立流程。"));
	AddNode(ENodeLibraryFilter::Skill, UYogFlowNode_TriggerDamageDealt::StaticClass(), LOCTEXT("NodeTriggerDamageDealtName", "造成伤害时"), LOCTEXT("NodeTriggerDamageDealtDescription", "用于月光、穿透、攻击命中强化等命中后触发逻辑。"));
	AddNode(ENodeLibraryFilter::Skill, UYogFlowNode_TriggerDamageReceived::StaticClass(), LOCTEXT("NodeTriggerDamageReceivedName", "受到伤害时"), LOCTEXT("NodeTriggerDamageReceivedDescription", "用于护盾、反击、受伤减伤等被击中触发逻辑。"));
	AddNode(ENodeLibraryFilter::Skill, UYogFlowNode_TriggerCritHit::StaticClass(), LOCTEXT("NodeTriggerCritHitName", "暴击时"), LOCTEXT("NodeTriggerCritHitDescription", "用于暴击追加伤害、状态或表现。"));
	AddNode(ENodeLibraryFilter::Skill, UYogFlowNode_TriggerKill::StaticClass(), LOCTEXT("NodeTriggerKillName", "击杀时"), LOCTEXT("NodeTriggerKillDescription", "用于击杀回血、爆炸、连锁触发。"));
	AddNode(ENodeLibraryFilter::Skill, UYogFlowNode_TriggerDash::StaticClass(), LOCTEXT("NodeTriggerDashName", "冲刺时"), LOCTEXT("NodeTriggerDashDescription", "用于冲刺残影、月光影、冲刺施法等。"));
	AddNode(ENodeLibraryFilter::Skill, UYogFlowNode_TriggerGameplayEvent::StaticClass(), LOCTEXT("NodeTriggerGameplayEventName", "等待事件"), LOCTEXT("NodeTriggerGameplayEventDescription", "持续监听 GameplayEvent，每次收到都触发 Out；用于充能命中、引爆等循环等待流程。"));
	AddNode(ENodeLibraryFilter::Skill, UYogFlowNode_EffectSendGameplayEvent::StaticClass(), LOCTEXT("NodeEffectSendGameplayEventName", "发送事件"), LOCTEXT("NodeEffectSendGameplayEventDescription", "向目标 ASC 发送 GameplayEvent，触发监听该事件的 GA；支持事件强度数据引脚传值。"));

	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectDamage::StaticClass(), LOCTEXT("NodeEffectDamageName", "伤害"), LOCTEXT("NodeEffectDamageDescription", "对目标造成一次直接伤害。"));
	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectHeal::StaticClass(), LOCTEXT("NodeEffectHealName", "治疗"), LOCTEXT("NodeEffectHealDescription", "恢复目标属性值，可作为瞬时治疗效果。"));
	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectCost::StaticClass(), LOCTEXT("NodeEffectCostName", "消耗"), LOCTEXT("NodeEffectCostDescription", "扣除法力、能量、弹药等资源。"));
	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectAttributeModify::StaticClass(), LOCTEXT("NodeEffectAttributeModifyName", "属性修改"), LOCTEXT("NodeEffectAttributeModifyDescription", "修改属性数值，适合增益、减益、护盾等持续效果。"));
	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectApplyState::StaticClass(), LOCTEXT("NodeEffectApplyStateName", "施加状态"), LOCTEXT("NodeEffectApplyStateDescription", "施加燃烧、中毒、流血、撕裂、诅咒等 GameplayEffect。"));
	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectApplyInRadius::StaticClass(), LOCTEXT("NodeEffectApplyInRadiusName", "范围施加GE"), LOCTEXT("NodeEffectApplyInRadiusDescription", "向半径内目标施加 GameplayEffect。"));
	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectAreaDamage::StaticClass(), LOCTEXT("NodeEffectAreaDamageName", "范围伤害"), LOCTEXT("NodeEffectAreaDamageDescription", "对范围目标造成伤害，适合燃烧地面、爆炸等。"));
	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectAddTag::StaticClass(), LOCTEXT("NodeEffectAddTagName", "添加Tag"), LOCTEXT("NodeEffectAddTagDescription", "给目标或拥有者添加状态 Tag。"));
	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectRemoveTag::StaticClass(), LOCTEXT("NodeEffectRemoveTagName", "移除Tag"), LOCTEXT("NodeEffectRemoveTagDescription", "移除状态 Tag，用于净化、终止状态或流程清理。"));
	AddNode(ENodeLibraryFilter::Effect, UYogFlowNode_EffectGrantAbility::StaticClass(), LOCTEXT("NodeEffectGrantAbilityName", "授予能力"), LOCTEXT("NodeEffectGrantAbilityDescription", "临时授予 GA，适合特殊动作、终结技或被动能力。"));

	AddNode(ENodeLibraryFilter::Task, UYogFlowNode_TaskSearchTarget::StaticClass(), LOCTEXT("NodeTaskSearchTargetName", "搜索目标"), LOCTEXT("NodeTaskSearchTargetDescription", "筛选或确认技能作用目标。"));
	AddNode(ENodeLibraryFilter::Task, UYogFlowNode_TaskEndSkill::StaticClass(), LOCTEXT("NodeTaskEndSkillName", "结束技能"), LOCTEXT("NodeTaskEndSkillDescription", "结束当前技能流程。"));
	AddNode(ENodeLibraryFilter::Task, UYogFlowNode_TaskPlayAnimation::StaticClass(), LOCTEXT("NodeTaskPlayAnimationName", "动画"), LOCTEXT("NodeTaskPlayAnimationDescription", "播放技能动作或蒙太奇。"));

	AddNode(ENodeLibraryFilter::Spawn, UYogFlowNode_SpawnAreaProfile::StaticClass(), LOCTEXT("NodeSpawnAreaProfileName", "生成区域配置"), LOCTEXT("NodeSpawnAreaProfileDescription", "生成可配置区域，适合燃烧圈、毒区、领域。"));
	AddNode(ENodeLibraryFilter::Spawn, UYogFlowNode_SpawnGroundPath::StaticClass(), LOCTEXT("NodeSpawnGroundPathName", "生成地面路径"), LOCTEXT("NodeSpawnGroundPathDescription", "生成路径类地面效果，适合燃烧轨迹或月光路径。"));
	AddNode(ENodeLibraryFilter::Spawn, UYogFlowNode_SpawnRangedProjectiles::StaticClass(), LOCTEXT("NodeSpawnRangedProjectilesName", "生成远程弹幕"), LOCTEXT("NodeSpawnRangedProjectilesDescription", "生成多枚远程投射物，支持连招增加数量和命中事件。"));
	AddNode(ENodeLibraryFilter::Spawn, UBFNode_GetProjectileModule::StaticClass(), LOCTEXT("NodeGetProjectileModuleName", "读取飞行物模块"), LOCTEXT("NodeGetProjectileModuleDescription", "读取符文DA飞行物模块配置（数量/锥角/速度），输出数据引脚接到弹幕节点。"));
	AddNode(ENodeLibraryFilter::Spawn, UBFNode_GetAuraModule::StaticClass(), LOCTEXT("NodeGetAuraModuleName", "读取光环模块"), LOCTEXT("NodeGetAuraModuleDescription", "读取符文DA光环/场地模块配置（长宽高/时间/间隔），输出数据引脚接到路径效果节点。"));

	AddNode(ENodeLibraryFilter::Condition, UYogFlowNode_ConditionAttributeCompare::StaticClass(), LOCTEXT("NodeConditionAttributeCompareName", "属性比较"), LOCTEXT("NodeConditionAttributeCompareDescription", "比较属性数值，并按结果分支。"));
	AddNode(ENodeLibraryFilter::Condition, UYogFlowNode_ConditionHasTag::StaticClass(), LOCTEXT("NodeConditionHasTagName", "拥有Tag"), LOCTEXT("NodeConditionHasTagDescription", "判断目标或拥有者是否有燃烧、中毒、月光等 Tag。"));
	AddNode(ENodeLibraryFilter::Condition, UYogFlowNode_ConditionProbability::StaticClass(), LOCTEXT("NodeConditionProbabilityName", "概率判断"), LOCTEXT("NodeConditionProbabilityDescription", "按概率触发，可用于暴击追加、随机状态。"));
	AddNode(ENodeLibraryFilter::Condition, UYogFlowNode_ConditionDoOnce::StaticClass(), LOCTEXT("NodeConditionDoOnceName", "只执行一次"), LOCTEXT("NodeConditionDoOnceDescription", "限制一次性触发，避免周期或连锁重复执行。"));
	AddNode(ENodeLibraryFilter::Condition, UYogFlowNode_ConditionCheckDistance::StaticClass(), LOCTEXT("NodeConditionCheckDistanceName", "距离判断"), LOCTEXT("NodeConditionCheckDistanceDescription", "按距离筛选或分支，适合近远程差异效果。"));
	AddNode(ENodeLibraryFilter::Condition, UBFNode_CombatCardContextBranch::StaticClass(), LOCTEXT("NodeConditionCombatCardContextName", "卡牌判断"), LOCTEXT("NodeConditionCombatCardContextDescription", "按卡牌类型、终结技、正反连携、Card.ID/Card.Effect 标签分支。"));
	AddNode(ENodeLibraryFilter::Condition, UBFNode_CompareFloat::StaticClass(), LOCTEXT("NodeCompareFloatName", "比较数值"), LOCTEXT("NodeCompareFloatDescription", "比较两个浮点数（>、>=、==、<=、<、!=），结果分 True/False 两路；A 支持数据引脚连线。"));
	AddNode(ENodeLibraryFilter::Condition, UBFNode_MathFloat::StaticClass(), LOCTEXT("NodeMathFloatName", "浮点运算"), LOCTEXT("NodeMathFloatDescription", "对两个浮点数做 +、-、×、÷ 运算，结果输出数据引脚，可接到伤害/倍率/阈值等字段。"));

	AddNode(ENodeLibraryFilter::Presentation, UYogFlowNode_PresentationPlayVFX::StaticClass(), LOCTEXT("NodePresentationPlayVFXName", "Niagara特效"), LOCTEXT("NodePresentationPlayVFXDescription", "播放 Niagara 表现。"));
	AddNode(ENodeLibraryFilter::Presentation, UYogFlowNode_PresentationCueOnActor::StaticClass(), LOCTEXT("NodePresentationCueOnActorName", "Cue到角色"), LOCTEXT("NodePresentationCueOnActorDescription", "在角色身上触发 GameplayCue。"));
	AddNode(ENodeLibraryFilter::Presentation, UYogFlowNode_PresentationCueAtLocation::StaticClass(), LOCTEXT("NodePresentationCueAtLocationName", "Cue到位置"), LOCTEXT("NodePresentationCueAtLocationDescription", "在世界位置触发 GameplayCue。"));
	AddNode(ENodeLibraryFilter::Presentation, UYogFlowNode_PresentationFlipbook::StaticClass(), LOCTEXT("NodePresentationFlipbookName", "序列帧特效"), LOCTEXT("NodePresentationFlipbookDescription", "播放 Flipbook 类表现。"));

	AddNode(ENodeLibraryFilter::Lifecycle, UYogFlowNode_LifecycleDelay::StaticClass(), LOCTEXT("NodeLifecycleDelayName", "延迟"), LOCTEXT("NodeLifecycleDelayDescription", "延迟后继续流程。"));
	AddNode(ENodeLibraryFilter::Lifecycle, UYogFlowNode_LifecycleFinishBuff::StaticClass(), LOCTEXT("NodeLifecycleFinishBuffName", "结束符文"), LOCTEXT("NodeLifecycleFinishBuffDescription", "主动结束当前符文 Buff 生命周期。"));
	AddNode(ENodeLibraryFilter::Pure, UBFNode_Pure_CombatCardContext::StaticClass(), LOCTEXT("NodePureCombatCardContextName", "卡牌信息"), LOCTEXT("NodePureCombatCardContextDesc", "输出当前攻击卡、终结技、连携方向、倍率、Card.ID/Card.Effect 等数据。"));
	AddNode(ENodeLibraryFilter::Pure, UBFNode_Pure_TuningValue::StaticClass(), LOCTEXT("NodePureTuningName", "读取数值"), LOCTEXT("NodePureTuningDesc", "输出数值表中某个 Key 的值，无执行引脚，拖线连接到效果节点的参数槽。"));
	AddNode(ENodeLibraryFilter::Pure, UBFNode_Pure_ComboIndex::StaticClass(), LOCTEXT("NodePureComboName", "连击段数"), LOCTEXT("NodePureComboDesc", "输出当前连击段数，无执行引脚，可连接到伤害倍率等数值槽。"));

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 8.f)
		[
			FilterWrap
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				NodeWrap
			]
		];
}

TSharedRef<SWidget> SRuneEditorWidget::BuildValidationPanel()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(0.f, 0.f, 0.f, 10.f)
		[
			MakeDetailLine(LOCTEXT("DiagnosticsValidation", "校验状态"), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SRuneEditorWidget::GetValidationSummaryText)))
		]
		+ SScrollBox::Slot()
		[
			SNew(STextBlock)
			.Text(this, &SRuneEditorWidget::GetValidationDetailsText)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.AutoWrapText(true)
		];
}

TSharedRef<SWidget> SRuneEditorWidget::BuildRunLogPanel()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			MakeDetailLine(LOCTEXT("DiagnosticsRunFeedback", "运行反馈"), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SRuneEditorWidget::GetRunFeedbackText)))
		];
}

TSharedRef<SWidget> SRuneEditorWidget::BuildSelectedNodePanel()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(0.f, 0.f, 0.f, 10.f)
		[
			MakeDetailLine(LOCTEXT("DiagnosticsSelectedNode", "当前节点"), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SRuneEditorWidget::GetSelectedFlowNodeText)))
		]
		+ SScrollBox::Slot()
		.Padding(0.f, 0.f, 0.f, 10.f)
		[
			MakeDetailLine(LOCTEXT("DiagnosticsSelectedNodeDescription", "说明"), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SRuneEditorWidget::GetSelectedFlowNodeDescriptionText)))
		]
		+ SScrollBox::Slot()
		[
			MakeDetailLine(LOCTEXT("DiagnosticsSelectedNodeOutgoing", "输出连接"), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SRuneEditorWidget::GetSelectedFlowNodeOutgoingText)))
		];
}

TSharedRef<SWidget> SRuneEditorWidget::BuildNodeLibraryItem(ENodeLibraryFilter Filter, UClass* NodeClass, const FText& DisplayName, const FText& Description)
{
	const FText ClassName = NodeClass ? FText::FromString(NodeClass->GetName()) : FText::GetEmpty();

	return SNew(SBox)
		.WidthOverride(250.f)
		.HeightOverride(136.f)
		.Visibility_Lambda([this, Filter]()
		{
			return ActiveNodeLibraryFilter == ENodeLibraryFilter::All || ActiveNodeLibraryFilter == Filter
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			.Padding(10.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(DisplayName)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(ClassName)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(Description)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				[
					SNew(SSpacer)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				[
					SNew(SButton)
					.Text(LOCTEXT("AddNodeFromLibrary", "添加"))
					.IsEnabled_Lambda([this, NodeClass]()
					{
						return NodeClass && GetSelectedFlowAsset() != nullptr;
					})
					.OnClicked(this, &SRuneEditorWidget::OnAddNodeFromLibrary, NodeClass)
				]
			]
		];
}

TSharedRef<SWidget> SRuneEditorWidget::BuildDetailsPanel()
{
	const FText InitialName = GetSelectedRuneNameText();
	const FText InitialTag = GetSelectedRuneTagText();
	const FText InitialSummary = GetSelectedSummaryText();

	FDetailsViewArgs NodeDetailsArgs;
	NodeDetailsArgs.bHideSelectionTip = true;
	NodeDetailsArgs.bShowOptions = false;
	NodeDetailsArgs.bShowScrollBar = true;
	NodeDetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	NodeDetailsView = PropertyEditorModule.CreateDetailView(NodeDetailsArgs);
	SyncNodeInspector();

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(8.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 6.f, 0.f)
				[
					BuildDetailsPanelTabButton(LOCTEXT("BasicInfoTab", "基础信息"), EDetailsPanelTab::BasicInfo)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					BuildDetailsPanelTabButton(LOCTEXT("CombatCardTab", "卡牌配置"), EDetailsPanelTab::CombatCard)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SAssignNew(DetailsPanelSwitcher, SWidgetSwitcher)
				.WidgetIndex(static_cast<int32>(ActiveDetailsTab))
				+ SWidgetSwitcher::Slot()
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					.Padding(0.f, 0.f, 0.f, 10.f)
					[
						MakeSection(
							LOCTEXT("BaseInfoSection", "基础信息"),
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.f, 0.f, 0.f, 8.f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.f, 0.f, 0.f, 3.f)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("EditableRuneNameLabel", "名称"))
									.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SAssignNew(SelectedRuneNameTextBox, SEditableTextBox)
									.Text(InitialName)
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.f, 0.f, 0.f, 8.f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.f, 0.f, 0.f, 3.f)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("EditableRuneIdTagLabel", "符文标签"))
									.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SAssignNew(SelectedRuneTagTextBox, SEditableTextBox)
									.Text(InitialTag)
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.f, 0.f, 0.f, 8.f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.f, 0.f, 0.f, 3.f)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("TypeLabel", "类型"))
									.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SAssignNew(RuneTypeCombo, FStringCombo)
									.OptionsSource(&RuneTypeOptions)
									.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
									.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo)
									{
										if (SelectInfo == ESelectInfo::Direct || !Value.IsValid()) return;
										URuneDataAsset* Rune = GetSelectedRune();
										if (!Rune) return;
										const FScopedTransaction Transaction(LOCTEXT("SetRuneType", "Set Rune Type"));
										Rune->Modify();
										Rune->RuneInfo.RuneConfig.RuneType = RuneTypeFromString(*Value);
										Rune->MarkPackageDirty();
										StatusText = FText::Format(LOCTEXT("RuneTypeChanged", "类型已设置为 {0}。"), FText::FromString(*Value));
									})
									.OnGenerateWidget_Lambda([](TSharedPtr<FString> Value)
									{
										return SNew(STextBlock).Text(FText::FromString(*Value)).Margin(FMargin(4.f, 2.f));
									})
									[
										SNew(STextBlock)
										.Text_Lambda([this]()
										{
											const URuneDataAsset* Rune = GetSelectedRune();
											return Rune ? FText::FromString(RuneTypeToString(Rune->GetRuneType())) : FText::GetEmpty();
										})
									]
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.f, 0.f, 0.f, 8.f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.f, 0.f, 0.f, 3.f)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("RarityLabel", "品质"))
									.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SAssignNew(RarityCombo, FStringCombo)
									.OptionsSource(&RarityOptions)
									.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
									.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo)
									{
										if (SelectInfo == ESelectInfo::Direct || !Value.IsValid()) return;
										URuneDataAsset* Rune = GetSelectedRune();
										if (!Rune) return;
										const FScopedTransaction Transaction(LOCTEXT("SetRarity", "Set Rune Rarity"));
										Rune->Modify();
										Rune->RuneInfo.RuneConfig.Rarity = RarityFromDisplayString(*Value);
										Rune->MarkPackageDirty();
										StatusText = FText::Format(LOCTEXT("RarityChanged", "品质已设置为 {0}。"), FText::FromString(*Value));
									})
									.OnGenerateWidget_Lambda([](TSharedPtr<FString> Value)
									{
										return SNew(STextBlock).Text(FText::FromString(*Value)).Margin(FMargin(4.f, 2.f));
									})
									[
										SNew(STextBlock)
										.Text_Lambda([this]()
										{
											const URuneDataAsset* Rune = GetSelectedRune();
											return Rune ? FText::FromString(RarityToDisplayString(Rune->GetRarity())) : FText::GetEmpty();
										})
									]
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.f, 0.f, 0.f, 8.f)
							[
								MakeDetailLine(LOCTEXT("LibraryCategoryLabel", "资源分类"), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SRuneEditorWidget::GetRuneLibraryCategoryText)))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.f, 0.f, 0.f, 8.f)
							[
								SNew(SWrapBox)
								.UseAllottedSize(true)
								+ SWrapBox::Slot()
								.Padding(0.f, 0.f, 6.f, 6.f)
								[
									BuildLibraryCategoryToggle(LOCTEXT("LibraryCategoryBase", "基础节点"), RequestOptionalTag(TEXT("Rune.Library.Base")))
								]
								+ SWrapBox::Slot()
								.Padding(0.f, 0.f, 6.f, 6.f)
								[
									BuildLibraryCategoryToggle(LOCTEXT("LibraryCategoryEnemy", "敌人"), RequestOptionalTag(TEXT("Rune.Library.Enemy")))
								]
								+ SWrapBox::Slot()
								.Padding(0.f, 0.f, 6.f, 6.f)
								[
									BuildLibraryCategoryToggle(LOCTEXT("LibraryCategoryLevel", "关卡"), RequestOptionalTag(TEXT("Rune.Library.Level")))
								]
								+ SWrapBox::Slot()
								.Padding(0.f, 0.f, 6.f, 6.f)
								[
									BuildLibraryCategoryToggle(LOCTEXT("LibraryCategoryFinisher", "终结技"), RequestOptionalTag(TEXT("Rune.Library.Finisher")))
								]
								+ SWrapBox::Slot()
								.Padding(0.f, 0.f, 6.f, 6.f)
								[
									BuildLibraryCategoryToggle(LOCTEXT("LibraryCategoryComboCard", "连携卡牌"), RequestOptionalTag(TEXT("Rune.Library.ComboCard")))
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SButton)
								.Text(LOCTEXT("SaveBasicInfoButton", "保存基础信息"))
								.IsEnabled_Lambda([this]()
								{
									return GetSelectedRune() != nullptr;
								})
								.OnClicked(this, &SRuneEditorWidget::OnSaveBasicInfoClicked)
							])
					]
					+ SScrollBox::Slot()
					.Padding(0.f, 0.f, 0.f, 10.f)
					[
						MakeSection(
							LOCTEXT("RuntimeSection", "触发与运行"),
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.f, 0.f, 0.f, 8.f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.f, 0.f, 0.f, 3.f)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("TriggerLabel", "触发方式"))
									.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SAssignNew(TriggerTypeCombo, FStringCombo)
									.OptionsSource(&TriggerTypeOptions)
									.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
									.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo)
									{
										if (SelectInfo == ESelectInfo::Direct || !Value.IsValid()) return;
										URuneDataAsset* Rune = GetSelectedRune();
										if (!Rune) return;
										const FScopedTransaction Transaction(LOCTEXT("SetTrigger", "Set Rune Trigger"));
										Rune->Modify();
										Rune->RuneInfo.RuneConfig.TriggerType = TriggerFromString(*Value);
										Rune->MarkPackageDirty();
										StatusText = FText::Format(LOCTEXT("TriggerChanged", "触发方式已设置为 {0}。"), FText::FromString(*Value));
									})
									.OnGenerateWidget_Lambda([](TSharedPtr<FString> Value)
									{
										return SNew(STextBlock).Text(FText::FromString(*Value)).Margin(FMargin(4.f, 2.f));
									})
									[
										SNew(STextBlock)
										.Text_Lambda([this]()
										{
											const URuneDataAsset* Rune = GetSelectedRune();
											return Rune ? FText::FromString(TriggerToString(Rune->GetTriggerType())) : FText::GetEmpty();
										})
									]
								]
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.f, 0.f, 0.f, 8.f)
							[
								MakeDetailLine(LOCTEXT("DurationLabel", "生命周期"), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SRuneEditorWidget::GetSelectedDurationText)))
							])
					]
					+ SScrollBox::Slot()
					.Padding(0.f, 0.f, 0.f, 10.f)
					[
						MakeSection(
							LOCTEXT("NodeInspectorSection", "节点属性"),
							SNew(SBox)
							.MinDesiredHeight(360.f)
							[
								NodeDetailsView.ToSharedRef()
							])
					]
					+ SScrollBox::Slot()
					[
						MakeSection(
							LOCTEXT("SummarySection", "界面摘要"),
							SNew(SBox)
							.MinDesiredHeight(180.f)
							[
								SAssignNew(SelectedSummaryTextBox, SMultiLineEditableTextBox)
								.Text(InitialSummary)
								.AutoWrapText(true)
							])
					]
				]
				+ SWidgetSwitcher::Slot()
				[
					BuildCombatCardPanel()
				]
			]
		];
}

TSharedRef<SWidget> SRuneEditorWidget::BuildComboRecipePanel()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f, 6.f, 8.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("DetailsView.CategoryTop")))
			.Padding(FMargin(6.f, 4.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.22f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ColNeighborTag", "邻接条件标签"))
					.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.37f)
				.Padding(4.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ColForward", "正向 (Forward)"))
					.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.37f)
				.Padding(4.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ColBackward", "反向 (Backward)"))
					.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(28.f)
				]
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(8.f, 2.f)
		[
			SAssignNew(ComboRecipeListView, SListView<FComboRecipeRowPtr>)
			.ListItemsSource(&ComboRecipeRows)
			.OnGenerateRow(this, &SRuneEditorWidget::BuildComboRecipeRow)
			.SelectionMode(ESelectionMode::None)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("AddRecipeRow", "+ 添加行"))
				.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
				.OnClicked(this, &SRuneEditorWidget::OnAddComboRecipeRowClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("SaveRecipeTable", "保存配方表"))
				.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
				.OnClicked(this, &SRuneEditorWidget::OnSaveComboRecipesClicked)
			]
		];
}

TSharedRef<ITableRow> SRuneEditorWidget::BuildComboRecipeRow(FComboRecipeRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	auto MakeDirectionCell = [this, Row](bool bForward) -> TSharedRef<SWidget>
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([Row, bForward]()
					{
						return (bForward ? Row->bHasForward : Row->bHasBackward)
							? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([Row, bForward](ECheckBoxState NewState)
					{
						(bForward ? Row->bHasForward : Row->bHasBackward) = (NewState == ECheckBoxState::Checked);
					})
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&FlowAssetNames)
					.IsEnabled_Lambda([Row, bForward]() { return bForward ? Row->bHasForward : Row->bHasBackward; })
					.OnSelectionChanged_Lambda([this, Row, bForward](TSharedPtr<FString> Item, ESelectInfo::Type)
					{
						if (!Item.IsValid()) return;
						const int32 Idx = FlowAssetNames.IndexOfByKey(Item);
						(bForward ? Row->ForwardFlowIdx : Row->BackwardFlowIdx) = FMath::Max(0, Idx);
					})
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
					{
						return SNew(STextBlock).Text(FText::FromString(*Item)).Margin(FMargin(4.f, 2.f));
					})
					[
						SNew(STextBlock)
						.Text_Lambda([this, Row, bForward]()
						{
							const int32 Idx = bForward ? Row->ForwardFlowIdx : Row->BackwardFlowIdx;
							return FlowAssetNames.IsValidIndex(Idx)
								? FText::FromString(*FlowAssetNames[Idx])
								: FText::FromString(TEXT("无"));
						})
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.f, 0.f, 0.f, 0.f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("→")))
					.ToolTipText(LOCTEXT("OpenLinkFlowTip", "在流程图面板中打开此连携 FA"))
					.IsEnabled_Lambda([Row, bForward]()
					{
						const int32 Idx = bForward ? Row->ForwardFlowIdx : Row->BackwardFlowIdx;
						return Idx > 0;
					})
					.OnClicked_Lambda([this, Row, bForward]()
					{
						const int32 Idx = bForward ? Row->ForwardFlowIdx : Row->BackwardFlowIdx;
						return OnOpenComboLinkFlowClicked(Idx);
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.f, 0.f, 0.f, 0.f)
				[
					SNew(SBox)
					.WidthOverride(64.f)
					[
						SNew(SNumericEntryBox<float>)
						.AllowSpin(true)
						.MinValue(0.f)
						.MaxValue(10.f)
						.MinSliderValue(0.f)
						.MaxSliderValue(5.f)
						.IsEnabled_Lambda([Row, bForward]() { return bForward ? Row->bHasForward : Row->bHasBackward; })
						.Value_Lambda([Row, bForward]()
						{
							return TOptional<float>(bForward ? Row->ForwardMultiplier : Row->BackwardMultiplier);
						})
						.OnValueChanged_Lambda([Row, bForward](float NewVal)
						{
							(bForward ? Row->ForwardMultiplier : Row->BackwardMultiplier) = NewVal;
						})
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SEditableTextBox)
				.HintText(LOCTEXT("ReasonHint", "文案"))
				.IsEnabled_Lambda([Row, bForward]() { return bForward ? Row->bHasForward : Row->bHasBackward; })
				.Text_Lambda([Row, bForward]()
				{
					return FText::FromString(bForward ? Row->ForwardReason : Row->BackwardReason);
				})
				.OnTextCommitted_Lambda([Row, bForward](const FText& NewText, ETextCommit::Type)
				{
					(bForward ? Row->ForwardReason : Row->BackwardReason) = NewText.ToString();
				})
			];
	};

	return SNew(STableRow<FComboRecipeRowPtr>, OwnerTable)
		.Padding(FMargin(4.f, 3.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.22f)
			.VAlign(VAlign_Top)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 3.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([Row]()
						{
							return Row->bUseEffectTag ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
						})
						.OnCheckStateChanged_Lambda([Row](ECheckBoxState NewState)
						{
							Row->bUseEffectTag = NewState == ECheckBoxState::Checked;
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4.f, 0.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text_Lambda([Row]()
						{
							return Row->bUseEffectTag
								? LOCTEXT("NeighborEffectTagMode", "效果Tag")
								: LOCTEXT("NeighborIdTagMode", "ID Tag");
						})
						.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SEditableTextBox)
					.HintText(LOCTEXT("NeighborTagHint", "Card.Effect.XXX / Card.ID.XXX"))
					.Text_Lambda([Row]() { return FText::FromString(Row->NeighborTagString); })
					.OnTextCommitted_Lambda([Row](const FText& NewText, ETextCommit::Type)
					{
						Row->NeighborTagString = NewText.ToString();
					})
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.37f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeDirectionCell(true)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.37f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeDirectionCell(false)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(LOCTEXT("DeleteRecipeRow", "✕"))
				.ToolTipText(LOCTEXT("DeleteRecipeRowTip", "删除此行"))
				.OnClicked_Lambda([this, Row]()
				{
					ComboRecipeRows.Remove(Row);
					if (ComboRecipeListView.IsValid())
					{
						ComboRecipeListView->RequestListRefresh();
					}
					return FReply::Handled();
				})
			]
		];
}

void SRuneEditorWidget::RefreshFlowAssetOptions()
{
	FlowAssetDataList.Empty();
	FlowAssetNames.Empty();
	FlowAssetNames.Add(MakeShared<FString>(TEXT("无")));

	if (!FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
	{
		return;
	}

	IAssetRegistry& Reg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> AllAssets;
	FARFilter Filter;
	Filter.ClassPaths.Add(UFlowAsset::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.PackagePaths.Add(FName(TEXT("/Game/YogRuneEditor/Flows")));
	Filter.bRecursivePaths = true;
	Reg.GetAssets(Filter, AllAssets);

	AllAssets.Sort([](const FAssetData& A, const FAssetData& B)
	{
		return A.AssetName.LexicalLess(B.AssetName);
	});

	for (const FAssetData& AD : AllAssets)
	{
		FlowAssetDataList.Add(AD);
		FlowAssetNames.Add(MakeShared<FString>(AD.AssetName.ToString()));
	}
}

void SRuneEditorWidget::RefreshComboRecipeRows()
{
	ComboRecipeRows.Empty();

	const URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune)
	{
		if (ComboRecipeListView.IsValid())
		{
			ComboRecipeListView->RequestListRefresh();
		}
		return;
	}

	auto FindOrAddRow = [this](const FString& TagStr, bool bUseEffectTag) -> FComboRecipeRowPtr
	{
		for (const FComboRecipeRowPtr& Row : ComboRecipeRows)
		{
			if (Row->NeighborTagString == TagStr && Row->bUseEffectTag == bUseEffectTag)
			{
				return Row;
			}
		}
		FComboRecipeRowPtr NewRow = MakeShared<FComboRecipeEditorRow>();
		NewRow->NeighborTagString = TagStr;
		NewRow->bUseEffectTag = bUseEffectTag;
		ComboRecipeRows.Add(NewRow);
		return NewRow;
	};

	auto FindFlowIdx = [this](const UFlowAsset* FA) -> int32
	{
		if (!FA)
		{
			return 0;
		}
		const FName TargetName(*FA->GetName());
		for (int32 i = 0; i < FlowAssetDataList.Num(); i++)
		{
			if (FlowAssetDataList[i].AssetName == TargetName)
			{
				return i + 1;
			}
		}
		return 0;
	};

	for (const FCombatCardLinkRecipe& Recipe : Rune->RuneInfo.CombatCard.LinkRecipes)
	{
		FString TagStr;
		bool bUseEffectTag = true;
		if (!Recipe.Condition.RequiredNeighborEffectTags.IsEmpty())
		{
			TagStr = Recipe.Condition.RequiredNeighborEffectTags.First().ToString();
			bUseEffectTag = true;
		}
		else if (!Recipe.Condition.RequiredNeighborIdTags.IsEmpty())
		{
			TagStr = Recipe.Condition.RequiredNeighborIdTags.First().ToString();
			bUseEffectTag = false;
		}

		FComboRecipeRowPtr Row = FindOrAddRow(TagStr, bUseEffectTag);

		if (Recipe.Direction == ECombatCardLinkOrientation::Forward)
		{
			Row->bHasForward = true;
			Row->ForwardFlowIdx = FindFlowIdx(Recipe.LinkFlow.Get());
			Row->ForwardMultiplier = Recipe.Multiplier;
			Row->ForwardReason = Recipe.ReasonText.ToString();
		}
		else
		{
			Row->bHasBackward = true;
			Row->BackwardFlowIdx = FindFlowIdx(Recipe.LinkFlow.Get());
			Row->BackwardMultiplier = Recipe.Multiplier;
			Row->BackwardReason = Recipe.ReasonText.ToString();
		}
	}

	if (ComboRecipeListView.IsValid())
	{
		ComboRecipeListView->RequestListRefresh();
	}
}

FReply SRuneEditorWidget::OnAddComboRecipeRowClicked()
{
	ComboRecipeRows.Add(MakeShared<FComboRecipeEditorRow>());
	if (ComboRecipeListView.IsValid())
	{
		ComboRecipeListView->RequestListRefresh();
	}
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnSaveComboRecipesClicked()
{
	URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("SaveComboRecipes", "Save Combo Recipes"));
	Rune->Modify();
	Rune->RuneInfo.CombatCard.LinkRecipes.Empty();

	for (const FComboRecipeRowPtr& Row : ComboRecipeRows)
	{
		auto MakeRecipe = [this, &Row](bool bForward) -> FCombatCardLinkRecipe
		{
			FCombatCardLinkRecipe Recipe;
			Recipe.Direction = bForward ? ECombatCardLinkOrientation::Forward : ECombatCardLinkOrientation::Reversed;

			if (!Row->NeighborTagString.IsEmpty())
			{
				const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Row->NeighborTagString), false);
				if (Tag.IsValid())
				{
					if (Row->bUseEffectTag)
					{
						Recipe.Condition.RequiredNeighborEffectTags.AddTag(Tag);
					}
					else
					{
						Recipe.Condition.RequiredNeighborIdTags.AddTag(Tag);
					}
				}
			}

			const int32 FlowIdx = bForward ? Row->ForwardFlowIdx : Row->BackwardFlowIdx;
			if (FlowIdx > 0 && FlowAssetDataList.IsValidIndex(FlowIdx - 1))
			{
				Recipe.LinkFlow = Cast<UFlowAsset>(FlowAssetDataList[FlowIdx - 1].GetAsset());
			}

			Recipe.Multiplier = bForward ? Row->ForwardMultiplier : Row->BackwardMultiplier;
			Recipe.ReasonText = FText::FromString(bForward ? Row->ForwardReason : Row->BackwardReason);
			return Recipe;
		};

		if (Row->bHasForward)
		{
			Rune->RuneInfo.CombatCard.LinkRecipes.Add(MakeRecipe(true));
		}
		if (Row->bHasBackward)
		{
			Rune->RuneInfo.CombatCard.LinkRecipes.Add(MakeRecipe(false));
		}
	}

	Rune->MarkPackageDirty();
	StatusText = FText::Format(
		LOCTEXT("ComboRecipesSaved", "已保存 {0} 条连携配方。"),
		FText::AsNumber(Rune->RuneInfo.CombatCard.LinkRecipes.Num()));
	return FReply::Handled();
}

// ─────────────────────────────────────────────────────────────────
//  Modules 面板
// ─────────────────────────────────────────────────────────────────

TSharedRef<SWidget> SRuneEditorWidget::BuildModulesPanel()
{
	// 创建过滤到 "Modules" Category 的 DetailsView
	FDetailsViewArgs ModulesArgs;
	ModulesArgs.bAllowSearch = false;
	ModulesArgs.bHideSelectionTip = true;
	ModulesArgs.bShowOptions = false;
	ModulesArgs.bShowScrollBar = true;
	ModulesArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	ModulesDetailsView = PropertyEditor.CreateDetailView(ModulesArgs);

	// 只显示 Category 为 "Modules" 的属性
	ModulesDetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda(
		[](const FPropertyAndParent& PropertyAndParent) -> bool
		{
			const FString Category = PropertyAndParent.Property.GetMetaData(TEXT("Category"));
			return Category.StartsWith(TEXT("Modules"));
		}));

	// 若已有选中符文，立刻显示
	if (URuneDataAsset* Rune = GetSelectedRune())
	{
		ModulesDetailsView->SetObject(Rune);
	}

	return SNew(SVerticalBox)
		// 顶部说明栏
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f, 6.f, 8.f, 4.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ModulesPanelHint",
				"勾选启用对应模块后填写参数。在 FA 流程图中可用 GetProjectileModule / GetAuraModule 节点读取这些值。"))
			.AutoWrapText(true)
			.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.f)))
		]
		// IDetailsView 主体
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			ModulesDetailsView.ToSharedRef()
		]
		// 底部保存按钮
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.f, 4.f, 8.f, 8.f)
		[
			SNew(SButton)
			.Text(LOCTEXT("SaveModules", "保存模块配置"))
			.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
			.OnClicked(this, &SRuneEditorWidget::OnSaveModulesClicked)
		];
}

FReply SRuneEditorWidget::OnSaveModulesClicked()
{
	URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune)
	{
		return FReply::Handled();
	}
	// IDetailsView 已直接编辑 DA，这里只需标记脏
	const FScopedTransaction Transaction(LOCTEXT("SaveModules", "Save Rune Modules"));
	Rune->Modify();
	Rune->MarkPackageDirty();
	StatusText = LOCTEXT("ModulesSaved", "模块配置已保存。");
	return FReply::Handled();
}

TSharedRef<SWidget> SRuneEditorWidget::BuildDetailsPanelTabButton(const FText& Label, EDetailsPanelTab Tab)
{
	return SNew(SButton)
		.Text_Lambda([this, Label, Tab]()
		{
			return ActiveDetailsTab == Tab
				? FText::Format(LOCTEXT("ActiveDetailsPanelTabLabel", "● {0}"), Label)
				: Label;
		})
		.OnClicked(this, &SRuneEditorWidget::OnDetailsPanelTabSelected, Tab);
}

TSharedRef<SWidget> SRuneEditorWidget::BuildCombatCardPanel()
{
	auto MakeFieldRow = [](const FText& Label, const TSharedRef<SWidget>& Widget)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 3.f)
			[
				SNew(STextBlock)
				.Text(Label)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				Widget
			];
	};

	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(0.f, 0.f, 0.f, 10.f)
		[
			MakeSection(
				LOCTEXT("CardBasicSection", "战斗卡牌基础"),
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeFieldRow(
						LOCTEXT("CardIsCombatCardLabel", "是否战斗卡牌"),
						SNew(SButton)
						.Text(this, &SRuneEditorWidget::GetSelectedCardIsCombatCardText)
						.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
						.OnClicked(this, &SRuneEditorWidget::OnToggleIsCombatCardClicked))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeFieldRow(
						LOCTEXT("CardTypeLabel", "卡牌类型"),
						SAssignNew(CardTypeCombo, FStringCombo)
						.OptionsSource(&CardTypeOptions)
						.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
						.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo)
						{
							if (SelectInfo == ESelectInfo::Direct || !Value.IsValid()) return;
							URuneDataAsset* Rune = GetSelectedRune();
							if (!Rune) return;
							const FScopedTransaction Transaction(LOCTEXT("SetCardType", "Set Card Type"));
							Rune->Modify();
							Rune->RuneInfo.CombatCard.CardType = CombatCardTypeFromString(*Value);
							Rune->MarkPackageDirty();
							StatusText = FText::Format(LOCTEXT("CardTypeChanged", "卡牌类型已设置为 {0}。"), FText::FromString(*Value));
						})
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> Value)
						{
							return SNew(STextBlock).Text(FText::FromString(*Value)).Margin(FMargin(4.f, 2.f));
						})
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const URuneDataAsset* Rune = GetSelectedRune();
								return Rune ? FText::FromString(CombatCardTypeToString(Rune->RuneInfo.CombatCard.CardType)) : FText::GetEmpty();
							})
						])
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeFieldRow(
						LOCTEXT("CardIdTagLabel", "CardIdTag (Card.ID.*)"),
						SAssignNew(CardIdTagTextBox, SEditableTextBox)
						.HintText(LOCTEXT("CardIdTagHint", "Card.ID.MyCard")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeFieldRow(
						LOCTEXT("CardRequiredActionLabel", "需要的动作"),
						SAssignNew(RequiredActionCombo, FStringCombo)
						.OptionsSource(&RequiredActionOptions)
						.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
						.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo)
						{
							if (SelectInfo == ESelectInfo::Direct || !Value.IsValid()) return;
							URuneDataAsset* Rune = GetSelectedRune();
							if (!Rune) return;
							const FScopedTransaction Transaction(LOCTEXT("SetRequiredAction", "Set Card Required Action"));
							Rune->Modify();
							Rune->RuneInfo.CombatCard.RequiredAction = CardRequiredActionFromString(*Value);
							Rune->MarkPackageDirty();
							StatusText = FText::Format(LOCTEXT("RequiredActionChanged", "需求动作已设置为 {0}。"), FText::FromString(*Value));
						})
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> Value)
						{
							return SNew(STextBlock).Text(FText::FromString(*Value)).Margin(FMargin(4.f, 2.f));
						})
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const URuneDataAsset* Rune = GetSelectedRune();
								return Rune ? FText::FromString(CardRequiredActionToString(Rune->RuneInfo.CombatCard.RequiredAction)) : FText::GetEmpty();
							})
						])
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeFieldRow(
						LOCTEXT("CardTriggerTimingLabel", "触发时机"),
						SAssignNew(TriggerTimingCombo, FStringCombo)
						.OptionsSource(&TriggerTimingOptions)
						.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
						.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Value, ESelectInfo::Type SelectInfo)
						{
							if (SelectInfo == ESelectInfo::Direct || !Value.IsValid()) return;
							URuneDataAsset* Rune = GetSelectedRune();
							if (!Rune) return;
							const FScopedTransaction Transaction(LOCTEXT("SetTriggerTiming", "Set Card Trigger Timing"));
							Rune->Modify();
							Rune->RuneInfo.CombatCard.TriggerTiming = CardTriggerTimingFromString(*Value);
							Rune->MarkPackageDirty();
							StatusText = FText::Format(LOCTEXT("TriggerTimingChanged", "触发时机已设置为 {0}。"), FText::FromString(*Value));
						})
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> Value)
						{
							return SNew(STextBlock).Text(FText::FromString(*Value)).Margin(FMargin(4.f, 2.f));
						})
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const URuneDataAsset* Rune = GetSelectedRune();
								return Rune ? FText::FromString(CardTriggerTimingToString(Rune->RuneInfo.CombatCard.TriggerTiming)) : FText::GetEmpty();
							})
						])
				])
		]
		+ SScrollBox::Slot()
		.Padding(0.f, 0.f, 0.f, 10.f)
		[
			MakeSection(
				LOCTEXT("CardDisplaySection", "卡牌显示"),
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeFieldRow(
						LOCTEXT("CardDisplayNameLabel", "显示名"),
						SAssignNew(CardDisplayNameTextBox, SEditableTextBox)
						.HintText(LOCTEXT("CardDisplayNameHint", "卡牌显示名称")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 3.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CardHUDSummaryLabel", "HUD 摘要"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
						.MinDesiredHeight(80.f)
						[
							SAssignNew(CardHUDSummaryTextBox, SMultiLineEditableTextBox)
							.HintText(LOCTEXT("CardHUDSummaryHint", "玩家可见的卡牌效果说明（1-2行）"))
							.AutoWrapText(true)
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeFieldRow(
						LOCTEXT("CardHUDReasonLabel", "HUD 理由"),
						SAssignNew(CardHUDReasonTextBox, SEditableTextBox)
						.HintText(LOCTEXT("CardHUDReasonHint", "触发理由短文案")))
				])
		]
		+ SScrollBox::Slot()
		.Padding(0.f, 0.f, 0.f, 10.f)
		[
			MakeSection(
				LOCTEXT("CardComboScalingSection", "连击缩放"),
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeFieldRow(
						LOCTEXT("CardComboScalingLabel", "启用连击缩放"),
						SNew(SButton)
						.Text(this, &SRuneEditorWidget::GetSelectedCardComboScalingText)
						.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
						.OnClicked(this, &SRuneEditorWidget::OnToggleComboScalingClicked))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 3.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CardComboScalarPerIndexLabel", "每层倍率 (ComboScalarPerIndex)"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SNumericEntryBox<float>)
						.Value_Lambda([this]() -> TOptional<float>
						{
							const URuneDataAsset* Rune = GetSelectedRune();
							return Rune ? TOptional<float>(Rune->RuneInfo.CombatCard.ComboScalarPerIndex) : TOptional<float>();
						})
						.OnValueCommitted_Lambda([this](float NewValue, ETextCommit::Type)
						{
							URuneDataAsset* Rune = GetSelectedRune();
							if (!Rune) return;
							const FScopedTransaction Transaction(LOCTEXT("EditComboScalarPerIndex", "Edit Combo Scalar Per Index"));
							Rune->Modify();
							Rune->RuneInfo.CombatCard.ComboScalarPerIndex = NewValue;
							Rune->MarkPackageDirty();
						})
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 3.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CardMaxComboScalarLabel", "最大连击倍率 (MaxComboScalar)"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SNumericEntryBox<float>)
						.Value_Lambda([this]() -> TOptional<float>
						{
							const URuneDataAsset* Rune = GetSelectedRune();
							return Rune ? TOptional<float>(Rune->RuneInfo.CombatCard.MaxComboScalar) : TOptional<float>();
						})
						.OnValueCommitted_Lambda([this](float NewValue, ETextCommit::Type)
						{
							URuneDataAsset* Rune = GetSelectedRune();
							if (!Rune) return;
							const FScopedTransaction Transaction(LOCTEXT("EditMaxComboScalar", "Edit Max Combo Scalar"));
							Rune->Modify();
							Rune->RuneInfo.CombatCard.MaxComboScalar = NewValue;
							Rune->MarkPackageDirty();
						})
					]
				])
		]
		+ SScrollBox::Slot()
		[
			SNew(SButton)
			.Text(LOCTEXT("SaveCardInfoButton", "保存卡牌配置"))
			.IsEnabled_Lambda([this]() { return GetSelectedRune() != nullptr; })
			.OnClicked(this, &SRuneEditorWidget::OnSaveCardInfoClicked)
		];
}

TSharedRef<ITableRow> SRuneEditorWidget::GenerateRuneRow(FRuneRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	URuneDataAsset* Rune = Row.IsValid() ? Row->Asset.Get() : nullptr;
	const FText Name = Rune ? FText::FromName(Rune->GetRuneName()) : LOCTEXT("MissingRune", "Missing Rune");
	const FText RuneTag = Rune ? FText::FromString(Rune->GetRuneIdTag().ToString()) : FText::GetEmpty();
	const FText Meta = Rune
		? FText::Format(LOCTEXT("RuneRowMeta", "{0} | {1} | {2} | {3}"),
			FText::FromString(RuneTypeToString(Rune->GetRuneType())),
			FText::FromString(RarityToDisplayString(Rune->GetRarity())),
			FText::FromString(TriggerToString(Rune->GetTriggerType())),
			FText::FromString(LibraryCategoryToString(Rune->GetLibraryTags())))
		: FText::GetEmpty();

	return SNew(STableRow<FRuneRowPtr>, OwnerTable)
		.Padding(4.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(Name)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(RuneTag)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(Meta)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
}

TSharedRef<ITableRow> SRuneEditorWidget::BuildTuningRow(FTuningRowPtr Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	const int32 RowIndex = Row.IsValid() ? Row->Index : INDEX_NONE;
	auto EditScalar = [this, RowIndex](TFunctionRef<void(FRuneTuningScalar&)> Mutator)
	{
		URuneDataAsset* Rune = GetSelectedRune();
		FRuneTuningScalar* Scalar = GetMutableTuningScalar(Rune, RowIndex);
		if (!Rune || !Scalar)
		{
			return;
		}

		Rune->Modify();
		Mutator(*Scalar);
		Rune->MarkPackageDirty();
		StatusText = LOCTEXT("TuningRowEditedStatus", "已更新符文数值表。");
	};

	auto GetScalarText = [this, RowIndex](TFunctionRef<FText(const FRuneTuningScalar&)> Getter)
	{
		const FRuneTuningScalar* Scalar = GetTuningScalar(GetSelectedRune(), RowIndex);
		return Scalar ? Getter(*Scalar) : FText::GetEmpty();
	};

	auto BuildTextCell = [](const TSharedRef<SWidget>& Widget)
	{
		return SNew(SBox)
			.MinDesiredWidth(90.f)
			[
				Widget
			];
	};

	return SNew(STableRow<FTuningRowPtr>, OwnerTable)
		.Padding(2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.8f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					BuildTextCell(
						SNew(SEditableTextBox)
						.Text_Lambda([GetScalarText]()
						{
							return GetScalarText([](const FRuneTuningScalar& Scalar)
							{
								return FText::FromName(Scalar.Key);
							});
						})
						.OnTextCommitted_Lambda([EditScalar](const FText& Text, ETextCommit::Type)
						{
							EditScalar([&Text](FRuneTuningScalar& Scalar)
							{
								Scalar.Key = FName(*Text.ToString());
							});
						}))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(2.f, 0.f, 0.f, 0.f)
				[
					SNew(SComboButton)
					.HasDownArrow(false)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ToolTipText(LOCTEXT("TuningKeyPickerTip", "从预定义 Key 列表中选择"))
					.ButtonContent()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("▾")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
					.OnGetMenuContent_Lambda([EditScalar, this]() -> TSharedRef<SWidget>
					{
						FMenuBuilder MenuBuilder(true, nullptr);
						for (const FPredefinedKeyGroup& Group : GetPredefinedTuningKeyGroups())
						{
							MenuBuilder.BeginSection(NAME_None, FText::FromString(Group.SectionName));
							for (const FName& Key : Group.Keys)
							{
								const FName KeyCopy = Key;
								MenuBuilder.AddMenuEntry(
									FText::FromName(KeyCopy),
									FText::GetEmpty(),
									FSlateIcon(),
									FUIAction(FExecuteAction::CreateLambda([EditScalar, KeyCopy]()
									{
										EditScalar([KeyCopy](FRuneTuningScalar& Scalar)
										{
											Scalar.Key = KeyCopy;
										});
									})));
							}
							MenuBuilder.EndSection();
						}
						return MenuBuilder.MakeWidget();
					})
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				BuildTextCell(
					SNew(SEditableTextBox)
					.Text_Lambda([GetScalarText]()
					{
						return GetScalarText([](const FRuneTuningScalar& Scalar)
						{
							return Scalar.DisplayName;
						});
					})
					.OnTextCommitted_Lambda([EditScalar](const FText& Text, ETextCommit::Type)
					{
						EditScalar([&Text](FRuneTuningScalar& Scalar)
						{
							Scalar.DisplayName = Text;
						});
					}))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.8f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				BuildTextCell(
					SNew(SEditableTextBox)
					.Text_Lambda([GetScalarText]()
					{
						return GetScalarText([](const FRuneTuningScalar& Scalar)
						{
							return FText::FromName(Scalar.Category);
						});
					})
					.OnTextCommitted_Lambda([EditScalar](const FText& Text, ETextCommit::Type)
					{
						EditScalar([&Text](FRuneTuningScalar& Scalar)
						{
							Scalar.Category = FName(*Text.ToString());
						});
					}))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.7f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&ValueSourceOptions)
				.OnSelectionChanged_Lambda([EditScalar, this](TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
				{
					if (SelectInfo == ESelectInfo::Direct || !Item.IsValid()) return;
					EditScalar([Item](FRuneTuningScalar& Scalar)
					{
						Scalar.ValueSource = ValueSourceFromString(*Item);
					});
				})
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(*Item)).Margin(FMargin(4.f, 2.f));
				})
				[
					SNew(STextBlock)
					.Text_Lambda([GetScalarText]()
					{
						return GetScalarText([](const FRuneTuningScalar& Scalar)
						{
							return TuningValueSourceToText(Scalar.ValueSource);
						});
					})
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.65f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				.BorderBackgroundColor_Lambda([this, RowIndex]()
				{
					const FRuneTuningScalar* S = GetTuningScalar(GetSelectedRune(), RowIndex);
					const bool bActive = S && S->ValueSource == ERuneTuningValueSource::Literal;
					return bActive ? FLinearColor(0.15f, 0.35f, 0.15f, 1.f) : FLinearColor(0.08f, 0.08f, 0.08f, 0.5f);
				})
				.Padding(2.f)
				[
					SNew(SNumericEntryBox<float>)
					.Value_Lambda([this, RowIndex]() -> TOptional<float>
					{
						const FRuneTuningScalar* Scalar = GetTuningScalar(GetSelectedRune(), RowIndex);
						return Scalar ? TOptional<float>(Scalar->Value) : TOptional<float>();
					})
					.OnValueCommitted_Lambda([EditScalar](float NewValue, ETextCommit::Type)
					{
						EditScalar([NewValue](FRuneTuningScalar& Scalar)
						{
							Scalar.Value = NewValue;
						});
					})
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.1f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				.BorderBackgroundColor_Lambda([this, RowIndex]()
				{
					const FRuneTuningScalar* S = GetTuningScalar(GetSelectedRune(), RowIndex);
					const bool bActive = S && S->ValueSource == ERuneTuningValueSource::Formula;
					return bActive ? FLinearColor(0.15f, 0.25f, 0.4f, 1.f) : FLinearColor(0.08f, 0.08f, 0.08f, 0.5f);
				})
				.Padding(2.f)
				[
					BuildTextCell(
						SNew(SEditableTextBox)
						.Text_Lambda([GetScalarText]()
						{
							return GetScalarText([](const FRuneTuningScalar& Scalar)
							{
								return FText::FromString(Scalar.FormulaExpression);
							});
						})
						.OnTextCommitted_Lambda([EditScalar](const FText& Text, ETextCommit::Type)
						{
							EditScalar([&Text](FRuneTuningScalar& Scalar)
							{
								Scalar.FormulaExpression = Text.ToString();
							});
						}))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.9f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				.BorderBackgroundColor_Lambda([this, RowIndex]()
				{
					const FRuneTuningScalar* S = GetTuningScalar(GetSelectedRune(), RowIndex);
					const bool bActive = S && S->ValueSource == ERuneTuningValueSource::Context;
					return bActive ? FLinearColor(0.35f, 0.2f, 0.1f, 1.f) : FLinearColor(0.08f, 0.08f, 0.08f, 0.5f);
				})
				.Padding(2.f)
				[
					BuildTextCell(
						SNew(SEditableTextBox)
						.Text_Lambda([GetScalarText]()
						{
							return GetScalarText([](const FRuneTuningScalar& Scalar)
							{
								return FText::FromName(Scalar.ContextKey);
							});
						})
						.OnTextCommitted_Lambda([EditScalar](const FText& Text, ETextCommit::Type)
						{
							EditScalar([&Text](FRuneTuningScalar& Scalar)
							{
								Scalar.ContextKey = FName(*Text.ToString());
							});
						}))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.55f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SNumericEntryBox<float>)
				.Value_Lambda([this, RowIndex]() -> TOptional<float>
				{
					const FRuneTuningScalar* Scalar = GetTuningScalar(GetSelectedRune(), RowIndex);
					return Scalar ? TOptional<float>(Scalar->MinValue) : TOptional<float>();
				})
				.OnValueCommitted_Lambda([EditScalar](float NewValue, ETextCommit::Type)
				{
					EditScalar([NewValue](FRuneTuningScalar& Scalar)
					{
						Scalar.MinValue = NewValue;
					});
				})
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.55f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SNumericEntryBox<float>)
				.Value_Lambda([this, RowIndex]() -> TOptional<float>
				{
					const FRuneTuningScalar* Scalar = GetTuningScalar(GetSelectedRune(), RowIndex);
					return Scalar ? TOptional<float>(Scalar->MaxValue) : TOptional<float>();
				})
				.OnValueCommitted_Lambda([EditScalar](float NewValue, ETextCommit::Type)
				{
					EditScalar([NewValue](FRuneTuningScalar& Scalar)
					{
						Scalar.MaxValue = NewValue;
					});
				})
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.9f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				BuildTextCell(
					SNew(SEditableTextBox)
					.Text_Lambda([GetScalarText]()
					{
						return GetScalarText([](const FRuneTuningScalar& Scalar)
						{
							return FText::FromName(Scalar.ValueTag.GetTagName());
						});
					})
					.OnTextCommitted_Lambda([EditScalar](const FText& Text, ETextCommit::Type)
					{
						EditScalar([&Text](FRuneTuningScalar& Scalar)
						{
							Scalar.ValueTag = FGameplayTag::RequestGameplayTag(FName(*Text.ToString()), false);
						});
					}))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.55f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				BuildTextCell(
					SNew(SEditableTextBox)
					.Text_Lambda([GetScalarText]()
					{
						return GetScalarText([](const FRuneTuningScalar& Scalar)
						{
							return Scalar.UnitText;
						});
					})
					.OnTextCommitted_Lambda([EditScalar](const FText& Text, ETextCommit::Type)
					{
						EditScalar([&Text](FRuneTuningScalar& Scalar)
						{
							Scalar.UnitText = Text;
						});
					}))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.2f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				BuildTextCell(
					SNew(SEditableTextBox)
					.Text_Lambda([GetScalarText]()
					{
						return GetScalarText([](const FRuneTuningScalar& Scalar)
						{
							return Scalar.Description;
						});
					})
					.OnTextCommitted_Lambda([EditScalar](const FText& Text, ETextCommit::Type)
					{
						EditScalar([&Text](FRuneTuningScalar& Scalar)
						{
							Scalar.Description = Text;
						});
					}))
			]
			// Combo Bonus: Mode
			+ SHorizontalBox::Slot()
			.FillWidth(0.65f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				.BorderBackgroundColor_Lambda([this, RowIndex]()
				{
					const FRuneTuningScalar* S = GetTuningScalar(GetSelectedRune(), RowIndex);
					return (S && S->ComboBonus.IsEnabled())
						? FLinearColor(0.25f, 0.1f, 0.4f, 1.f)
						: FLinearColor(0.08f, 0.08f, 0.08f, 0.5f);
				})
				.Padding(2.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&ComboBonusModeOptions)
					.OnSelectionChanged_Lambda([EditScalar, this](TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
					{
						if (SelectInfo == ESelectInfo::Direct || !Item.IsValid()) return;
						EditScalar([Item](FRuneTuningScalar& Scalar)
						{
							Scalar.ComboBonus.Mode = ComboBonusModeFromString(*Item);
						});
					})
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
					{
						return SNew(STextBlock).Text(FText::FromString(*Item)).Margin(FMargin(4.f, 2.f));
					})
					[
						SNew(STextBlock)
						.Text_Lambda([GetScalarText]()
						{
							return GetScalarText([](const FRuneTuningScalar& Scalar)
							{
								return FText::FromString(ComboBonusModeToString(Scalar.ComboBonus.Mode));
							});
						})
					]
				]
			]
			// Combo Bonus: BonusPerStack
			+ SHorizontalBox::Slot()
			.FillWidth(0.6f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SNumericEntryBox<float>)
				.IsEnabled_Lambda([this, RowIndex]()
				{
					const FRuneTuningScalar* S = GetTuningScalar(GetSelectedRune(), RowIndex);
					return S && S->ComboBonus.IsEnabled();
				})
				.Value_Lambda([this, RowIndex]() -> TOptional<float>
				{
					const FRuneTuningScalar* Scalar = GetTuningScalar(GetSelectedRune(), RowIndex);
					return Scalar ? TOptional<float>(Scalar->ComboBonus.BonusPerStack) : TOptional<float>();
				})
				.OnValueCommitted_Lambda([EditScalar](float NewValue, ETextCommit::Type)
				{
					EditScalar([NewValue](FRuneTuningScalar& Scalar)
					{
						Scalar.ComboBonus.BonusPerStack = NewValue;
					});
				})
			]
			// Combo Bonus: MaxBonus
			+ SHorizontalBox::Slot()
			.FillWidth(0.6f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SNumericEntryBox<float>)
				.IsEnabled_Lambda([this, RowIndex]()
				{
					const FRuneTuningScalar* S = GetTuningScalar(GetSelectedRune(), RowIndex);
					return S && S->ComboBonus.IsEnabled();
				})
				.Value_Lambda([this, RowIndex]() -> TOptional<float>
				{
					const FRuneTuningScalar* Scalar = GetTuningScalar(GetSelectedRune(), RowIndex);
					return Scalar ? TOptional<float>(Scalar->ComboBonus.MaxBonus) : TOptional<float>();
				})
				.OnValueCommitted_Lambda([EditScalar](float NewValue, ETextCommit::Type)
				{
					EditScalar([NewValue](FRuneTuningScalar& Scalar)
					{
						Scalar.ComboBonus.MaxBonus = NewValue;
					});
				})
			]
			// Combo Bonus: RoundMode
			+ SHorizontalBox::Slot()
			.FillWidth(0.55f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&RoundModeOptions)
				.IsEnabled_Lambda([this, RowIndex]()
				{
					const FRuneTuningScalar* S = GetTuningScalar(GetSelectedRune(), RowIndex);
					return S && S->ComboBonus.IsEnabled();
				})
				.OnSelectionChanged_Lambda([EditScalar](TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
				{
					if (SelectInfo == ESelectInfo::Direct || !Item.IsValid()) return;
					EditScalar([Item](FRuneTuningScalar& Scalar)
					{
						Scalar.ComboBonus.RoundMode = RoundModeFromString(*Item);
					});
				})
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(*Item)).Margin(FMargin(4.f, 2.f));
				})
				[
					SNew(STextBlock)
					.Text_Lambda([GetScalarText]()
					{
						return GetScalarText([](const FRuneTuningScalar& Scalar)
						{
							return FText::FromString(RoundModeToString(Scalar.ComboBonus.RoundMode));
						});
					})
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("DeleteTuningRow", "删除"))
				.OnClicked(this, &SRuneEditorWidget::OnDeleteTuningRowClicked, RowIndex)
			]
		];
}

void SRuneEditorWidget::RefreshData(const FText& NewStatus)
{
	TArray<URuneDataAsset*> Runes = UDataEditorLibrary::GetAllRuneDAs();
	Runes.Sort([](const URuneDataAsset& A, const URuneDataAsset& B)
	{
		return A.GetName() < B.GetName();
	});

	RuneRows.Reset();
	for (URuneDataAsset* Rune : Runes)
	{
		if (!Rune)
		{
			continue;
		}

		const FString PackageName = Rune->GetOutermost() ? Rune->GetOutermost()->GetName() : FString();
		if (!PackageName.StartsWith(FRuneEditorAuthoring::GetDefaultAssetRoot()))
		{
			continue;
		}

		const FString SearchBlob = FString::Printf(
			TEXT("%s %s %s %s %s"),
			*Rune->GetName(),
			*Rune->GetRuneName().ToString(),
			*Rune->GetRuneIdTag().ToString(),
			*Rune->GetHUDSummaryText().ToString(),
			*LibraryCategoryToString(Rune->GetLibraryTags()));
		if (!SearchText.IsEmpty() && !SearchBlob.Contains(SearchText, ESearchCase::IgnoreCase))
		{
			continue;
		}
		if (!DoesRuneMatchResourceFilter(Rune))
		{
			continue;
		}

		RuneRows.Add(MakeShared<FRuneEditorRuneRow>(Rune));
	}

	if (!SelectedRune.IsValid() && RuneRows.Num() > 0)
	{
		SelectedRune = RuneRows[0]->Asset.Get();
		SelectedResource = SelectedRune.Get();
	}

	if (RuneListView.IsValid())
	{
		RuneListView->RequestListRefresh();
		for (const FRuneRowPtr& Row : RuneRows)
		{
			if (Row.IsValid() && Row->Asset == SelectedRune)
			{
				RuneListView->SetSelection(Row);
				break;
			}
		}
	}

	StatusText = NewStatus.IsEmpty()
		? FText::Format(LOCTEXT("RefreshStatus", "符文资源：{0}。节点菜单已限制为 Yog 专用节点。"),
			FText::AsNumber(RuneRows.Num()))
		: NewStatus;

	RefreshFlowNodes();
	RefreshTuningRows();
}

void SRuneEditorWidget::RefreshFlowNodes()
{
	FlowNodeRows.Reset();

	UFlowAsset* FlowAsset = GetSelectedFlowAsset();
	for (const FRuneEditorFlowNodeSummary& Summary : FRuneEditorFlowAuthoring::BuildFlowNodeSummaries(FlowAsset))
	{
		FlowNodeRows.Add(MakeShared<FRuneEditorFlowNodeRow>(Summary));
	}

	if (!SelectedFlowNode.IsValid() && FlowNodeRows.Num() > 0)
	{
		SelectedFlowNode = FlowNodeRows[0]->Summary.Node;
	}

	RebuildGraphEditor();
	SyncNodeInspector();
}

void SRuneEditorWidget::RefreshTuningRows()
{
	TuningRows.Reset();

	static const FName ComboOnlyFilter(TEXT("##ComboBonus"));

	if (const URuneDataAsset* Rune = GetSelectedRune())
	{
		const TArray<FRuneTuningScalar>& Scalars = Rune->GetTuningScalars();
		for (int32 Index = 0; Index < Scalars.Num(); ++Index)
		{
			const FRuneTuningScalar& Scalar = Scalars[Index];
			if (!TuningCategoryFilter.IsNone())
			{
				if (TuningCategoryFilter == ComboOnlyFilter)
				{
					if (!Scalar.ComboBonus.IsEnabled()) continue;
				}
				else if (Scalar.Category != TuningCategoryFilter)
				{
					continue;
				}
			}
			TuningRows.Add(MakeShared<FRuneEditorTuningRow>(Index));
		}
	}

	if (TuningListView.IsValid())
	{
		TuningListView->RequestListRefresh();
	}
}

TSharedRef<SWidget> SRuneEditorWidget::BuildTuningCategoryFilterBar()
{
	struct FFilterEntry { FText Label; FName Category; };
	static const FFilterEntry Entries[] = {
		{ LOCTEXT("TuningFilterAll",      "全部"),     NAME_None              },
		{ LOCTEXT("TuningFilterDamage",   "伤害"),     FName("Damage")        },
		{ LOCTEXT("TuningFilterProjectile","飞行物"),  FName("Projectile")    },
		{ LOCTEXT("TuningFilterStack",    "层数"),     FName("Stack")         },
		{ LOCTEXT("TuningFilterDuration", "持续时间"), FName("Duration")      },
		{ LOCTEXT("TuningFilterCombo",    "连招奖励"), FName("##ComboBonus")  },
	};

	TSharedRef<SHorizontalBox> Bar = SNew(SHorizontalBox);
	for (const FFilterEntry& E : Entries)
	{
		const FName Cat = E.Category;
		const FText Lbl = E.Label;
		Bar->AddSlot()
			.AutoWidth()
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(SButton)
				.Text_Lambda([this, Cat, Lbl]()
				{
					return TuningCategoryFilter == Cat
						? FText::Format(LOCTEXT("ActiveTuningFilter", "● {0}"), Lbl)
						: Lbl;
				})
				.OnClicked(this, &SRuneEditorWidget::OnTuningCategoryFilterClicked, Cat)
			];
	}
	return Bar;
}

FReply SRuneEditorWidget::OnTuningCategoryFilterClicked(FName Category)
{
	TuningCategoryFilter = Category;
	RefreshTuningRows();
	return FReply::Handled();
}

void SRuneEditorWidget::BindGraphEditorCommands()
{
	if (GraphEditorCommands.IsValid())
	{
		return;
	}

	FGenericCommands::Register();
	GraphEditorCommands = MakeShared<FUICommandList>();
	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &SRuneEditorWidget::DeleteSelectedGraphNodes),
		FCanExecuteAction::CreateSP(this, &SRuneEditorWidget::CanDeleteSelectedGraphNodes));
}

void SRuneEditorWidget::RebuildGraphEditor(UFlowAsset* OverrideFlow)
{
	if (!GraphEditorContainer.IsValid())
	{
		return;
	}

	UFlowAsset* FlowAsset = OverrideFlow ? OverrideFlow : GetSelectedFlowAsset();
	DisplayedFlowAsset = FlowAsset;
	UEdGraph* GraphToEdit = FlowAsset ? FlowAsset->GetGraph() : nullptr;
	if (!GraphToEdit)
	{
		RuneGraphEditor.Reset();
		GraphEditorContainer->SetContent(
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			.Padding(12.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("NoGraphToEdit", "没有找到流程图。请先新建或选择一个 Yog 符文资源。"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.AutoWrapText(true)
			]);
		return;
	}

	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("RuneGraphCorner", "YOG 符文流程");

	SGraphEditor::FGraphEditorEvents GraphEvents;
	GraphEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &SRuneEditorWidget::OnGraphSelectionChanged);

	BindGraphEditorCommands();

	SAssignNew(RuneGraphEditor, SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.Appearance(AppearanceInfo)
		.GraphToEdit(GraphToEdit)
		.GraphEvents(GraphEvents)
		.AutoExpandActionMenu(true);

	GraphEditorContainer->SetContent(RuneGraphEditor.ToSharedRef());
	RuneGraphEditor->ZoomToFit(false);

	if (UFlowNode* FlowNode = GetSelectedFlowNode())
	{
		if (UEdGraphNode* GraphNode = FlowNode->GetGraphNode())
		{
			RuneGraphEditor->SetNodeSelection(GraphNode, true);
		}
	}
}

void SRuneEditorWidget::DeleteSelectedGraphNodes()
{
	if (!RuneGraphEditor.IsValid() || GEditor->PlayWorld != nullptr)
	{
		return;
	}

	UFlowAsset* FlowAsset = GetSelectedFlowAsset();
	UEdGraph* Graph = FlowAsset ? FlowAsset->GetGraph() : RuneGraphEditor->GetCurrentGraph();
	if (!Graph)
	{
		return;
	}

	const FGraphPanelSelectionSet SelectedNodes = RuneGraphEditor->GetSelectedNodes();
	if (SelectedNodes.Num() == 0)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteRuneFlowNodes", "删除符文流程节点"));
	Graph->Modify();
	if (FlowAsset)
	{
		FlowAsset->Modify();
	}

	bool bDeletedAnyNode = false;
	SelectedFlowNode.Reset();
	RuneGraphEditor->ClearSelectionSet();

	for (UObject* SelectedObject : SelectedNodes)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject);
		if (!Node || !Node->CanUserDeleteNode())
		{
			continue;
		}

		const UFlowGraphNode* FlowGraphNode = Cast<UFlowGraphNode>(Node);
		UFlowNode* FlowNode = FlowGraphNode ? Cast<UFlowNode>(FlowGraphNode->GetFlowNodeBase()) : nullptr;

		if (const UEdGraphSchema* Schema = Graph->GetSchema())
		{
			Schema->BreakNodeLinks(*Node);
		}
		Node->DestroyNode();

		if (FlowAsset && FlowNode)
		{
			FlowAsset->UnregisterNode(FlowNode->GetGuid());
		}
		bDeletedAnyNode = true;
	}

	if (!bDeletedAnyNode)
	{
		return;
	}

	if (FlowAsset)
	{
		FlowAsset->PostEditChange();
	}
	Graph->NotifyGraphChanged();
	RefreshFlowNodes();
	StatusText = LOCTEXT("DeleteGraphNodesStatus", "已删除选中的符文流程节点。");
}

bool SRuneEditorWidget::CanDeleteSelectedGraphNodes() const
{
	if (!RuneGraphEditor.IsValid() || GEditor->PlayWorld != nullptr)
	{
		return false;
	}

	const FGraphPanelSelectionSet SelectedNodes = RuneGraphEditor->GetSelectedNodes();
	for (UObject* SelectedObject : SelectedNodes)
	{
		const UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject);
		if (Node && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}

void SRuneEditorWidget::OnSearchTextChanged(const FText& NewText)
{
	SearchText = NewText.ToString();
	RefreshData(LOCTEXT("SearchStatus", "Rune list filtered."));
}

void SRuneEditorWidget::OnRuneSelectionChanged(FRuneRowPtr Row, ESelectInfo::Type SelectInfo)
{
	SelectedRune = Row.IsValid() ? Row->Asset.Get() : nullptr;
	SelectedResource = SelectedRune.Get();
	SelectedFlowNode.Reset();
	RunFeedbackText = LOCTEXT("RunFeedbackSelectionChanged", "当前选择尚未运行符文。");
	RefreshFlowNodes();
	RefreshTuningRows();
	RebuildGraphEditor();
	SyncSelectedRuneEditorFields();
	StatusText = SelectedRune.IsValid()
		? FText::Format(LOCTEXT("SelectionStatus", "已选择 {0}。"), GetSelectedRuneNameText())
		: LOCTEXT("NoSelectionStatus", "未选择符文。");
}

FReply SRuneEditorWidget::OnBottomTabSelected(EBottomPanelTab Tab)
{
	ActiveBottomTab = Tab;
	if (BottomPanelSwitcher.IsValid())
	{
		BottomPanelSwitcher->SetActiveWidgetIndex(static_cast<int32>(Tab));
	}
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnCenterTabSelected(ECenterPanelTab Tab)
{
	ActiveCenterTab = Tab;
	if (CenterPanelSwitcher.IsValid())
	{
		CenterPanelSwitcher->SetActiveWidgetIndex(static_cast<int32>(Tab));
	}
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnResourceFilterSelected(EResourceFilter Filter)
{
	ActiveResourceFilter = Filter;
	RefreshData(LOCTEXT("ResourceFilterStatus", "已更新符文资源筛选。"));
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnLibraryCategoryToggled(FGameplayTag CategoryTag)
{
	URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune || !CategoryTag.IsValid())
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("ToggleRuneLibraryCategory", "Toggle Rune Library Category"));
	Rune->Modify();

	FGameplayTagContainer& LibraryTags = Rune->RuneInfo.RuneConfig.LibraryTags;
	if (LibraryTags.HasTagExact(CategoryTag))
	{
		LibraryTags.RemoveTag(CategoryTag);
	}
	else
	{
		LibraryTags.AddTag(CategoryTag);
	}

	Rune->MarkPackageDirty();
	RefreshData(LOCTEXT("LibraryCategoryUpdated", "已更新符文资源分类。"));
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnNodeLibraryFilterSelected(ENodeLibraryFilter Filter)
{
	ActiveNodeLibraryFilter = Filter;
	StatusText = LOCTEXT("NodeLibraryFilterStatus", "已更新节点库筛选。");
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnAddNodeFromLibrary(UClass* NodeClass)
{
	FText Message;
	UFlowNode* NewNode = FRuneEditorFlowAuthoring::AddNodeAfter(GetSelectedFlowAsset(), GetSelectedFlowNode(), NodeClass, Message);
	StatusText = Message;

	if (NewNode)
	{
		SelectedFlowNode = NewNode;
		RefreshFlowNodes();
		SyncNodeInspector();
		if (RuneGraphEditor.IsValid())
		{
			if (UEdGraphNode* GraphNode = NewNode->GetGraphNode())
			{
				RuneGraphEditor->ClearSelectionSet();
				RuneGraphEditor->SetNodeSelection(GraphNode, true);
				RuneGraphEditor->JumpToNode(GraphNode, false);
			}
		}
	}

	return FReply::Handled();
}

void SRuneEditorWidget::OnGraphSelectionChanged(const TSet<UObject*>& Nodes)
{
	SelectedFlowNode.Reset();

	for (UObject* NodeObject : Nodes)
	{
		if (const UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(NodeObject))
		{
			if (UFlowNode* FlowNode = Cast<UFlowNode>(GraphNode->GetFlowNodeBase()))
			{
				SelectedFlowNode = FlowNode;
				break;
			}
		}
	}

	SyncNodeInspector();
	StatusText = SelectedFlowNode.IsValid()
		? FText::Format(LOCTEXT("GraphSelectionStatus", "已选择图表节点 {0}。"), GetSelectedFlowNodeText())
		: LOCTEXT("GraphNoSelectionStatus", "未选择图表节点。");
}

FReply SRuneEditorWidget::OnRefreshClicked()
{
	RefreshData(LOCTEXT("ManualRefreshStatus", "符文编辑器已刷新。"));
	SyncSelectedRuneEditorFields();
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnCreateRuneClicked()
{
	FRuneEditorCreateRuneRequest Request;
	Request.DisplayName = GetNewRuneNameText();
	Request.RuneIdTag = GetNewRuneTagText();
	Request.AssetFolder = GetNewRuneFolderText();
	Request.FlowAssetFolder = FRuneEditorAuthoring::GetDefaultFlowFolder();
	Request.RuneType = CreateRuneType;
	Request.Rarity = CreateRarity;
	Request.TriggerType = CreateTriggerType;
	Request.SummaryText = FText::FromString(TEXT("通过符文编辑器创建。"));
	Request.bOpenAfterCreate = false;

	FRuneEditorCreateRuneResult Result = FRuneEditorAuthoring::CreateRuneAuthoringAssets(Request);
	if (Result.bSuccess)
	{
		SelectedRune = Result.RuneAsset.Get();
		SelectedResource = SelectedRune.Get();
		RefreshData(Result.Message);
		SyncSelectedRuneEditorFields();
		if (RuneListView.IsValid())
		{
			for (const FRuneRowPtr& Row : RuneRows)
			{
				if (Row.IsValid() && Row->Asset == SelectedRune)
				{
					RuneListView->SetSelection(Row);
					break;
				}
			}
		}
	}
	else
	{
		StatusText = Result.Message;
	}
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnCopyAssetClicked()
{
	UObject* Resource = GetSelectedResource();
	CopiedResource = Resource;
	StatusText = Resource
		? FText::Format(LOCTEXT("CopiedResourceStatus", "已复制资源引用 {0}。"), FText::FromString(Resource->GetName()))
		: LOCTEXT("CopyNoResourceStatus", "没有可复制的资源。");
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnPasteAssetClicked()
{
	FRuneEditorAssetActionResult Result = FRuneEditorAuthoring::DuplicateResource(CopiedResource.Get());
	if (Result.bSuccess && Result.Assets.Num() > 0)
	{
		SelectResourceAsset(Result.Assets[0].Get());
	}
	StatusText = Result.Message;
	RefreshData(StatusText);
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnRenameAssetClicked()
{
	UObject* Resource = GetSelectedResource();
	const FString NewName = ResourceRenameTextBox.IsValid() ? ResourceRenameTextBox->GetText().ToString() : FString();
	StatusText = FRuneEditorAuthoring::RenameResource(Resource, NewName);
	RefreshData(StatusText);
	SyncSelectedRuneEditorFields();
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnDeleteAssetClicked()
{
	UObject* Resource = GetSelectedResource();
	StatusText = FRuneEditorAuthoring::DeleteResources({ Resource });
	if (Resource && Resource == SelectedResource.Get())
	{
		SelectedResource.Reset();
	}
	if (Resource && Resource == SelectedRune.Get())
	{
		SelectedRune.Reset();
	}
	SelectedFlowNode.Reset();
	RefreshData(StatusText);
	SyncSelectedRuneEditorFields();
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnLocateAssetClicked()
{
	UObject* Resource = GetSelectedResource();
	FRuneEditorAuthoring::SyncBrowserToAssets({ Resource });
	StatusText = Resource
		? FText::Format(LOCTEXT("LocateResourceStatus", "已在内容浏览器中定位 {0}。"), FText::FromString(Resource->GetName()))
		: LOCTEXT("LocateNoResourceStatus", "没有可定位的资源。");
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnSaveBasicInfoClicked()
{
	URuneDataAsset* Rune = GetSelectedRune();
	const FString DisplayName = SelectedRuneNameTextBox.IsValid() ? SelectedRuneNameTextBox->GetText().ToString() : FString();
	const FString RuneIdTag = SelectedRuneTagTextBox.IsValid() ? SelectedRuneTagTextBox->GetText().ToString() : FString();
	const FText SummaryText = SelectedSummaryTextBox.IsValid() ? SelectedSummaryTextBox->GetText() : FText::GetEmpty();

	StatusText = FRuneEditorAuthoring::SaveRuneBasicInfo(Rune, DisplayName, RuneIdTag, SummaryText);
	RefreshData(StatusText);
	SyncSelectedRuneEditorFields();
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnAddTuningRowClicked()
{
	URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AddRuneTuningRow", "Add Rune Tuning Row"));
	Rune->Modify();

	FRuneTuningScalar& NewScalar = Rune->RuneInfo.RuneConfig.TuningScalars.AddDefaulted_GetRef();
	NewScalar.Key = FName(TEXT("NewValue"));
	NewScalar.DisplayName = LOCTEXT("NewTuningRowDisplayName", "新数值");
	NewScalar.Category = FName(TEXT("通用"));
	NewScalar.ValueSource = ERuneTuningValueSource::Literal;
	NewScalar.Value = 0.f;

	Rune->MarkPackageDirty();
	RefreshTuningRows();
	StatusText = LOCTEXT("AddTuningRowStatus", "已新增符文数值行。");
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnInsertTuningPresetClicked(const FString& GroupName)
{
	URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune) { return FReply::Handled(); }

	const TArray<FTuningPresetGroup>& Presets = GetTuningPresets();
	const FTuningPresetGroup* Found = Presets.FindByPredicate([&GroupName](const FTuningPresetGroup& G)
	{
		return G.Name == GroupName;
	});
	if (!Found || Found->Rows.IsEmpty()) { return FReply::Handled(); }

	const FScopedTransaction Transaction(LOCTEXT("InsertTuningPreset", "Insert Tuning Preset"));
	Rune->Modify();
	for (const FTuningPresetRow& PresetRow : Found->Rows)
	{
		Rune->RuneInfo.RuneConfig.TuningScalars.Add(PresetRow.ToScalar());
	}
	Rune->MarkPackageDirty();
	RefreshTuningRows();
	StatusText = FText::Format(LOCTEXT("InsertPresetStatus", "已插入「{0}」预设，共 {1} 行。"),
		FText::FromString(GroupName),
		FText::AsNumber(Found->Rows.Num()));
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnDeleteTuningRowClicked(int32 RowIndex)
{
	URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune || !Rune->RuneInfo.RuneConfig.TuningScalars.IsValidIndex(RowIndex))
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteRuneTuningRow", "Delete Rune Tuning Row"));
	Rune->Modify();
	Rune->RuneInfo.RuneConfig.TuningScalars.RemoveAt(RowIndex);
	Rune->MarkPackageDirty();
	RefreshTuningRows();
	StatusText = LOCTEXT("DeleteTuningRowStatus", "已删除符文数值行。");
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnTuningSourceClicked(int32 RowIndex)
{
	URuneDataAsset* Rune = GetSelectedRune();
	FRuneTuningScalar* Scalar = GetMutableTuningScalar(Rune, RowIndex);
	if (!Rune || !Scalar)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("ChangeRuneTuningSource", "Change Rune Tuning Source"));
	Rune->Modify();
	Scalar->ValueSource = GetNextTuningValueSource(Scalar->ValueSource);
	Rune->MarkPackageDirty();
	StatusText = FText::Format(LOCTEXT("TuningSourceChangedStatus", "数值方式已切换为 {0}。"), TuningValueSourceToText(Scalar->ValueSource));
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnRunRuneClicked()
{
	const FRuneEditorRunRuneResult Result = FRuneEditorAuthoring::RunRuneOnSelectedActor(GetSelectedRune());
	StatusText = Result.Message;
	RunFeedbackText = Result.DebugText;
	OnBottomTabSelected(EBottomPanelTab::RunLog);
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnOpenRuneClicked() const
{
	OpenAsset(GetSelectedRune());
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnOpenFlowClicked() const
{
	OpenAsset(GetSelectedFlowAsset());
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnOpenComboLinkFlowClicked(int32 FlowIdx)
{
	if (FlowIdx <= 0 || !FlowAssetDataList.IsValidIndex(FlowIdx - 1))
	{
		return FReply::Handled();
	}
	UObject* Asset = FlowAssetDataList[FlowIdx - 1].GetAsset();
	if (UFlowAsset* FA = Cast<UFlowAsset>(Asset))
	{
		OnCenterTabSelected(ECenterPanelTab::FlowGraph);
		RebuildGraphEditor(FA);
	}
	return FReply::Handled();
}

FReply SRuneEditorWidget::SelectFlowNode(UFlowNode* FlowNode)
{
	SelectedFlowNode = FlowNode;
	SyncNodeInspector();

	if (RuneGraphEditor.IsValid() && FlowNode)
	{
		if (UEdGraphNode* GraphNode = FlowNode->GetGraphNode())
		{
			RuneGraphEditor->ClearSelectionSet();
			RuneGraphEditor->SetNodeSelection(GraphNode, true);
			RuneGraphEditor->JumpToNode(GraphNode, false);
		}
	}

	StatusText = FlowNode
		? FText::Format(LOCTEXT("GraphNodeSelectionStatus", "已选择流程节点 {0}。"), GetSelectedFlowNodeText())
		: LOCTEXT("NoGraphNodeSelectionStatus", "未选择流程节点。");
	return FReply::Handled();
}

FText SRuneEditorWidget::GetStatusText() const
{
	return StatusText;
}

FText SRuneEditorWidget::GetSelectedRuneNameText() const
{
	const URuneDataAsset* Rune = GetSelectedRune();
	if (Rune)
	{
		return FText::FromName(Rune->GetRuneName());
	}

	if (const UFlowAsset* FlowAsset = Cast<UFlowAsset>(GetSelectedResource()))
	{
		return FText::FromString(FlowAsset->GetName());
	}

	return LOCTEXT("NoRuneName", "未选择资源");
}

FText SRuneEditorWidget::GetSelectedRuneTagText() const
{
	const URuneDataAsset* Rune = GetSelectedRune();
	return Rune ? FText::FromString(Rune->GetRuneIdTag().ToString()) : FText::GetEmpty();
}

FText SRuneEditorWidget::GetRuneLibraryCategoryText() const
{
	const URuneDataAsset* Rune = GetSelectedRune();
	return Rune ? FText::FromString(LibraryCategoryToString(Rune->GetLibraryTags())) : FText::GetEmpty();
}

FText SRuneEditorWidget::GetSelectedDurationText() const
{
	const UFlowAsset* FlowAsset = GetSelectedFlowAsset();
	if (!FlowAsset)
	{
		return LOCTEXT("LifetimeMissingFlow", "未选择流程资产");
	}
	return LOCTEXT("LifetimeFlowOwned", "由流程图控制运行生命周期");
}

FText SRuneEditorWidget::GetSelectedFlowText() const
{
	const UFlowAsset* FlowAsset = GetSelectedFlowAsset();
	return FlowAsset ? FText::FromString(FlowAsset->GetName()) : LOCTEXT("NoFlowAsset", "未关联流程资产");
}

FText SRuneEditorWidget::GetSelectedFlowNodeText() const
{
	const UFlowNode* FlowNode = GetSelectedFlowNode();
	if (!FlowNode)
	{
		return LOCTEXT("NoFlowNodeSelected", "请在图表中选择一个流程节点。");
	}

	return FText::Format(
		LOCTEXT("SelectedFlowNodeText", "{0} | {1}"),
		FlowNode->GetNodeTitle(),
		FText::FromString(FlowNode->GetClass()->GetName()));
}

FText SRuneEditorWidget::GetSelectedSummaryText() const
{
	const URuneDataAsset* Rune = GetSelectedRune();
	return Rune ? Rune->GetHUDSummaryText() : FText::GetEmpty();
}

FText SRuneEditorWidget::GetValidationSummaryText() const
{
	return FRuneEditorValidation::ValidateRuneGraph(GetSelectedRune()).BuildSummaryText();
}

FText SRuneEditorWidget::GetValidationDetailsText() const
{
	return FRuneEditorValidation::ValidateRuneGraph(GetSelectedRune()).BuildDetailsText();
}

FText SRuneEditorWidget::GetRunFeedbackText() const
{
	return RunFeedbackText.IsEmpty()
		? LOCTEXT("RunFeedbackEmpty", "本次编辑器会话尚未运行符文。")
		: RunFeedbackText;
}

FText SRuneEditorWidget::GetSelectedFlowNodeDescriptionText() const
{
	const UFlowNode* FlowNode = GetSelectedFlowNode();
	return FlowNode ? FText::FromString(FlowNode->GetNodeDescription()) : LOCTEXT("NoFlowNodeDescription", "没有可显示的节点说明。");
}

FText SRuneEditorWidget::GetSelectedFlowNodeOutgoingText() const
{
	const UFlowNode* FlowNode = GetSelectedFlowNode();
	if (!FlowNode)
	{
		return LOCTEXT("NoFlowNodeOutgoing", "未选择节点。");
	}

	for (const FFlowNodeRowPtr& Row : FlowNodeRows)
	{
		if (Row.IsValid() && Row->Summary.Node == FlowNode)
		{
			return FText::FromString(Row->Summary.OutgoingSummary);
		}
	}

	return LOCTEXT("FlowNodeOutgoingUnknown", "暂无输出连接信息。");
}

EVisibility SRuneEditorWidget::GetHasSelectedRuneVisibility() const
{
	return GetSelectedRune() ? EVisibility::Visible : EVisibility::Collapsed;
}

void SRuneEditorWidget::OpenAsset(UObject* Asset) const
{
	if (!Asset || !GEditor)
	{
		return;
	}

	if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
	{
		AssetEditorSubsystem->OpenEditorForAsset(Asset);
	}
}

UObject* SRuneEditorWidget::GetSelectedResource() const
{
	if (SelectedResource.IsValid())
	{
		return SelectedResource.Get();
	}
	return SelectedRune.Get();
}

URuneDataAsset* SRuneEditorWidget::GetSelectedRune() const
{
	return SelectedRune.Get();
}

UFlowAsset* SRuneEditorWidget::GetSelectedFlowAsset() const
{
	if (const URuneDataAsset* Rune = GetSelectedRune())
	{
		return Rune->GetFlowAsset();
	}
	return Cast<UFlowAsset>(GetSelectedResource());
}

UFlowNode* SRuneEditorWidget::GetSelectedFlowNode() const
{
	return SelectedFlowNode.Get();
}

URuneDataAsset* SRuneEditorWidget::FindRuneForFlow(UFlowAsset* FlowAsset) const
{
	if (!FlowAsset)
	{
		return nullptr;
	}

	for (URuneDataAsset* Rune : UDataEditorLibrary::GetAllRuneDAs())
	{
		if (Rune && Rune->GetFlowAsset() == FlowAsset)
		{
			return Rune;
		}
	}

	return nullptr;
}

FGameplayTag SRuneEditorWidget::GetResourceFilterTag(EResourceFilter Filter) const
{
	switch (Filter)
	{
	case EResourceFilter::Base:
		return RequestOptionalTag(TEXT("Rune.Library.Base"));
	case EResourceFilter::Enemy:
		return RequestOptionalTag(TEXT("Rune.Library.Enemy"));
	case EResourceFilter::Level:
		return RequestOptionalTag(TEXT("Rune.Library.Level"));
	case EResourceFilter::Finisher:
		return RequestOptionalTag(TEXT("Rune.Library.Finisher"));
	case EResourceFilter::ComboCard:
		return RequestOptionalTag(TEXT("Rune.Library.ComboCard"));
	case EResourceFilter::All:
	default:
		return FGameplayTag();
	}
}

bool SRuneEditorWidget::DoesRuneMatchResourceFilter(const URuneDataAsset* Rune) const
{
	if (!Rune || ActiveResourceFilter == EResourceFilter::All)
	{
		return true;
	}

	const FGameplayTag FilterTag = GetResourceFilterTag(ActiveResourceFilter);
	if (FilterTag.IsValid() && Rune->GetLibraryTags().HasTagExact(FilterTag))
	{
		return true;
	}

	FString FilterBlob = FString::Printf(
		TEXT("%s %s %s %s %s"),
		*Rune->GetName(),
		*Rune->GetRuneName().ToString(),
		*Rune->GetRuneIdTag().ToString(),
		*Rune->GetHUDSummaryText().ToString(),
		*LibraryCategoryToString(Rune->GetLibraryTags()));

	auto ContainsAny = [&FilterBlob](std::initializer_list<const TCHAR*> Tokens)
	{
		for (const TCHAR* Token : Tokens)
		{
			if (FilterBlob.Contains(Token, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	};

	switch (ActiveResourceFilter)
	{
	case EResourceFilter::Base:
		return ContainsAny({ TEXT("基础"), TEXT("Base"), TEXT("Common"), TEXT("Template"), TEXT("通用") });
	case EResourceFilter::Enemy:
		return ContainsAny({ TEXT("敌人"), TEXT("Enemy"), TEXT("Monster") });
	case EResourceFilter::Level:
		return ContainsAny({ TEXT("关卡"), TEXT("Level"), TEXT("Stage"), TEXT("Map") });
	case EResourceFilter::Finisher:
		return ContainsAny({ TEXT("终结"), TEXT("Finisher"), TEXT("Ultimate") });
	case EResourceFilter::ComboCard:
		return ContainsAny({ TEXT("连携"), TEXT("Combo"), TEXT("Card"), TEXT("卡牌") });
	case EResourceFilter::All:
	default:
		return true;
	}
}

bool SRuneEditorWidget::IsRuneLibraryCategoryActive(FGameplayTag CategoryTag) const
{
	const URuneDataAsset* Rune = GetSelectedRune();
	return Rune && CategoryTag.IsValid() && Rune->GetLibraryTags().HasTagExact(CategoryTag);
}

void SRuneEditorWidget::SelectResourceAsset(UObject* Asset)
{
	SelectedResource = Asset;
	SelectedFlowNode.Reset();
	RunFeedbackText = LOCTEXT("RunFeedbackSelectionChanged", "当前选择尚未运行符文。");

	if (URuneDataAsset* Rune = Cast<URuneDataAsset>(Asset))
	{
		SelectedRune = Rune;
	}
	else if (UFlowAsset* FlowAsset = Cast<UFlowAsset>(Asset))
	{
		SelectedRune = FindRuneForFlow(FlowAsset);
	}
	else
	{
		SelectedRune.Reset();
	}

	RefreshFlowNodes();
	RefreshTuningRows();
	RebuildGraphEditor();
	SyncSelectedRuneEditorFields();
	SyncNodeInspector();
	StatusText = Asset
		? FText::Format(LOCTEXT("ResourceSelectionStatus", "已选择资源 {0}。"), FText::FromString(Asset->GetName()))
		: LOCTEXT("NoResourceSelectionStatus", "未选择资源。");
}

void SRuneEditorWidget::SyncSelectedRuneEditorFields()
{
	if (SelectedRuneNameTextBox.IsValid())
	{
		SelectedRuneNameTextBox->SetText(GetSelectedRuneNameText());
	}
	if (SelectedRuneTagTextBox.IsValid())
	{
		SelectedRuneTagTextBox->SetText(GetSelectedRuneTagText());
	}
	if (SelectedSummaryTextBox.IsValid())
	{
		SelectedSummaryTextBox->SetText(GetSelectedSummaryText());
	}

	const URuneDataAsset* Rune = GetSelectedRune();
	if (CardIdTagTextBox.IsValid())
	{
		CardIdTagTextBox->SetText(Rune
			? FText::FromString(Rune->RuneInfo.CombatCard.CardIdTag.ToString())
			: FText::GetEmpty());
	}
	if (CardDisplayNameTextBox.IsValid())
	{
		CardDisplayNameTextBox->SetText(Rune ? Rune->RuneInfo.CombatCard.DisplayName : FText::GetEmpty());
	}
	if (CardHUDSummaryTextBox.IsValid())
	{
		CardHUDSummaryTextBox->SetText(Rune ? Rune->RuneInfo.CombatCard.HUDSummaryText : FText::GetEmpty());
	}
	if (CardHUDReasonTextBox.IsValid())
	{
		CardHUDReasonTextBox->SetText(Rune ? Rune->RuneInfo.CombatCard.HUDReasonText : FText::GetEmpty());
	}

	// Sync combo box selections to match the current rune's data
	auto SyncCombo = [](TSharedPtr<FStringCombo>& Combo, const TArray<TSharedPtr<FString>>& Options, const FString& Current)
	{
		if (!Combo.IsValid()) return;
		for (const TSharedPtr<FString>& Opt : Options)
		{
			if (Opt.IsValid() && *Opt == Current)
			{
				Combo->SetSelectedItem(Opt);
				return;
			}
		}
	};

	if (Rune)
	{
		SyncCombo(RuneTypeCombo,    RuneTypeOptions,    RuneTypeToString(Rune->GetRuneType()));
		SyncCombo(RarityCombo,      RarityOptions,      RarityToDisplayString(Rune->GetRarity()));
		SyncCombo(TriggerTypeCombo, TriggerTypeOptions, TriggerToString(Rune->GetTriggerType()));
		SyncCombo(CardTypeCombo,       CardTypeOptions,       CombatCardTypeToString(Rune->RuneInfo.CombatCard.CardType));
		SyncCombo(RequiredActionCombo, RequiredActionOptions, CardRequiredActionToString(Rune->RuneInfo.CombatCard.RequiredAction));
		SyncCombo(TriggerTimingCombo,  TriggerTimingOptions,  CardTriggerTimingToString(Rune->RuneInfo.CombatCard.TriggerTiming));
	}

	RefreshComboRecipeRows();
}

void SRuneEditorWidget::SyncNodeInspector()
{
	if (NodeDetailsView.IsValid())
	{
		NodeDetailsView->SetObject(GetSelectedFlowNode());
	}
	if (ModulesDetailsView.IsValid())
	{
		ModulesDetailsView->SetObject(GetSelectedRune());
	}
}

FString SRuneEditorWidget::GetNewRuneNameText() const
{
	return NewRuneNameTextBox.IsValid() ? NewRuneNameTextBox->GetText().ToString() : TEXT("New Rune");
}

FString SRuneEditorWidget::GetNewRuneTagText() const
{
	if (NewRuneTagTextBox.IsValid() && !NewRuneTagTextBox->GetText().IsEmpty())
	{
		return NewRuneTagTextBox->GetText().ToString();
	}
	return FRuneEditorAuthoring::BuildDefaultRuneTagFromName(GetNewRuneNameText());
}

FString SRuneEditorWidget::GetNewRuneFolderText() const
{
	return NewRuneFolderTextBox.IsValid() ? NewRuneFolderTextBox->GetText().ToString() : FRuneEditorAuthoring::GetDefaultRuneFolder();
}

FReply SRuneEditorWidget::OnExportTuningClicked()
{
	const URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune)
	{
		return FReply::Handled();
	}

	// Header row
	TArray<FString> Lines;
	Lines.Add(TEXT("Key\t显示名\t分类\t数值方式\t具体值\t公式\t上下文\tMin\tMax\tValueTag\t单位\t说明"));

	for (const FRuneTuningScalar& S : Rune->GetTuningScalars())
	{
		Lines.Add(FString::Printf(
			TEXT("%s\t%s\t%s\t%s\t%f\t%s\t%s\t%f\t%f\t%s\t%s\t%s"),
			*S.Key.ToString(),
			*S.DisplayName.ToString(),
			*S.Category.ToString(),
			*TuningValueSourceToText(S.ValueSource).ToString(),
			S.Value,
			*S.FormulaExpression,
			*S.ContextKey.ToString(),
			S.MinValue,
			S.MaxValue,
			*S.ValueTag.ToString(),
			*S.UnitText.ToString(),
			*S.Description.ToString()));
	}

	const FString CSV = FString::Join(Lines, TEXT("\n"));
	FPlatformApplicationMisc::ClipboardCopy(*CSV);
	StatusText = FText::Format(
		LOCTEXT("ExportTuningStatus", "已导出 {0} 行数值到剪贴板。"),
		FText::AsNumber(Rune->GetTuningScalars().Num()));
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnImportTuningClicked()
{
	URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune)
	{
		return FReply::Handled();
	}

	FString CSV;
	FPlatformApplicationMisc::ClipboardPaste(CSV);
	if (CSV.IsEmpty())
	{
		StatusText = LOCTEXT("ImportTuningEmptyClipboard", "剪贴板为空，无法导入。");
		return FReply::Handled();
	}

	TArray<FString> Lines;
	CSV.ParseIntoArrayLines(Lines, false);

	// Skip header line
	if (Lines.Num() <= 1)
	{
		StatusText = LOCTEXT("ImportTuningNoData", "CSV 只有表头或为空，跳过导入。");
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("ImportTuning", "Import Rune Tuning CSV"));
	Rune->Modify();
	Rune->RuneInfo.RuneConfig.TuningScalars.Reset();

	int32 ImportedCount = 0;
	for (int32 i = 1; i < Lines.Num(); ++i)
	{
		TArray<FString> Cols;
		Lines[i].ParseIntoArray(Cols, TEXT("\t"), false);

		// Pad to at least 12 columns to handle trailing empties
		while (Cols.Num() < 12)
		{
			Cols.Add(FString());
		}

		FRuneTuningScalar& S = Rune->RuneInfo.RuneConfig.TuningScalars.AddDefaulted_GetRef();
		S.Key = FName(*Cols[0]);
		S.DisplayName = FText::FromString(Cols[1]);
		S.Category = FName(*Cols[2]);

		// ValueSource: match display text
		const FString SourceStr = Cols[3];
		if (SourceStr == TuningValueSourceToText(ERuneTuningValueSource::Formula).ToString())
		{
			S.ValueSource = ERuneTuningValueSource::Formula;
		}
		else if (SourceStr == TuningValueSourceToText(ERuneTuningValueSource::MMC).ToString())
		{
			S.ValueSource = ERuneTuningValueSource::MMC;
		}
		else if (SourceStr == TuningValueSourceToText(ERuneTuningValueSource::Context).ToString())
		{
			S.ValueSource = ERuneTuningValueSource::Context;
		}
		else
		{
			S.ValueSource = ERuneTuningValueSource::Literal;
		}

		S.Value = FCString::Atof(*Cols[4]);
		S.FormulaExpression = Cols[5];
		S.ContextKey = FName(*Cols[6]);
		S.MinValue = FCString::Atof(*Cols[7]);
		S.MaxValue = FCString::Atof(*Cols[8]);
		S.ValueTag = FGameplayTag::RequestGameplayTag(FName(*Cols[9]), false);
		S.UnitText = FText::FromString(Cols[10]);
		S.Description = FText::FromString(Cols[11]);
		++ImportedCount;
	}

	Rune->MarkPackageDirty();
	RefreshTuningRows();
	StatusText = FText::Format(
		LOCTEXT("ImportTuningStatus", "已从剪贴板导入 {0} 行数值。"),
		FText::AsNumber(ImportedCount));
	return FReply::Handled();
}


FReply SRuneEditorWidget::OnDetailsPanelTabSelected(EDetailsPanelTab Tab)
{
	ActiveDetailsTab = Tab;
	if (DetailsPanelSwitcher.IsValid())
	{
		DetailsPanelSwitcher->SetActiveWidgetIndex(static_cast<int32>(Tab));
	}
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnSaveCardInfoClicked()
{
	URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("SaveCardInfo", "Save Combat Card Info"));
	Rune->Modify();
	FCombatCardConfig& Card = Rune->RuneInfo.CombatCard;

	if (CardIdTagTextBox.IsValid())
	{
		FText TagStatus;
		Card.CardIdTag = FRuneEditorAuthoring::EnsureGameplayTag(CardIdTagTextBox->GetText().ToString(), TagStatus);
	}
	if (CardDisplayNameTextBox.IsValid())
	{
		Card.DisplayName = CardDisplayNameTextBox->GetText();
	}
	if (CardHUDSummaryTextBox.IsValid())
	{
		Card.HUDSummaryText = CardHUDSummaryTextBox->GetText();
	}
	if (CardHUDReasonTextBox.IsValid())
	{
		Card.HUDReasonText = CardHUDReasonTextBox->GetText();
	}

	Rune->MarkPackageDirty();
	StatusText = LOCTEXT("CardInfoSaved", "已保存卡牌配置。");
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnToggleIsCombatCardClicked()
{
	URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("ToggleIsCombatCard", "Toggle Is Combat Card"));
	Rune->Modify();
	Rune->RuneInfo.CombatCard.bIsCombatCard = !Rune->RuneInfo.CombatCard.bIsCombatCard;
	Rune->MarkPackageDirty();
	StatusText = Rune->RuneInfo.CombatCard.bIsCombatCard
		? LOCTEXT("CombatCardEnabled", "已启用战斗卡牌模式。")
		: LOCTEXT("CombatCardDisabled", "已禁用战斗卡牌模式。");
	return FReply::Handled();
}

FReply SRuneEditorWidget::OnToggleComboScalingClicked()
{
	URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune)
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("ToggleComboScaling", "Toggle Combo Scaling"));
	Rune->Modify();
	Rune->RuneInfo.CombatCard.bUseComboEffectScaling = !Rune->RuneInfo.CombatCard.bUseComboEffectScaling;
	Rune->MarkPackageDirty();
	StatusText = Rune->RuneInfo.CombatCard.bUseComboEffectScaling
		? LOCTEXT("ComboScalingEnabled", "已启用连击缩放。")
		: LOCTEXT("ComboScalingDisabled", "已禁用连击缩放。");
	return FReply::Handled();
}

FText SRuneEditorWidget::GetSelectedCardIsCombatCardText() const
{
	const URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune)
	{
		return FText::GetEmpty();
	}
	return Rune->RuneInfo.CombatCard.bIsCombatCard
		? LOCTEXT("IsCombatCardYes", "是（战斗卡牌）")
		: LOCTEXT("IsCombatCardNo", "否（非战斗卡牌）");
}

FText SRuneEditorWidget::GetSelectedCardComboScalingText() const
{
	const URuneDataAsset* Rune = GetSelectedRune();
	if (!Rune)
	{
		return FText::GetEmpty();
	}
	return Rune->RuneInfo.CombatCard.bUseComboEffectScaling
		? LOCTEXT("ComboScalingYes", "已启用")
		: LOCTEXT("ComboScalingNo", "已禁用");
}


#undef LOCTEXT_NAMESPACE
