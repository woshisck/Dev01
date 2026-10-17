#include "Tools/Workflow/SDevKitGraphicsQualityWorkbenchWidget.h"
#include "Tools/Workflow/DevKitWorkbenchUI.h"
#include "Tools/CharacterLookDev/DevKitLookDevLevelService.h"
#include "Engine/EngineTypes.h"
#include "Editor.h"

#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Scalability.h"
#include "RenderUtils.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "UnrealEdGlobals.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	constexpr int32 QualityCount = 4;
	constexpr int32 ParameterCount = 27;
	// Keep the legacy slot for migration only. CSM distance never controls VSM.
	bool IsRetiredShadowRow(int32 Row) { return Row == 3; }
	const TCHAR* PresetSection = TEXT("DevKit.GraphicsQualityWorkbench");
	const TCHAR* PresetKey = TEXT("Preset");

	struct FParameterDefinition
	{
		const TCHAR* CVar;
		const TCHAR* ScalabilityGroup;
		float ConstructorFallback;
		const TCHAR* Label;
		const TCHAR* Impact;
		float Min;
		float Max;
		float Delta;
		bool bInteger;
	};

	const FParameterDefinition ParameterDefinitions[ParameterCount] =
	{
		{TEXT("r.Lumen.ScreenProbeGather.RadianceCache.NumProbesToTraceBudget"), TEXT("GlobalIlluminationQuality"), 100.f, TEXT("Lumen 探针预算"), TEXT("影响间接光探测细节与 GPU 成本；仅在对应 Lumen 路径启用时生效。"), 0.f, 600.f, 1.f, true},
		{TEXT("r.Shadow.Virtual.MaxPhysicalPages"), TEXT("ShadowQuality"), 2048.f, TEXT("VSM 物理页容量"), TEXT("控制 VSM 页池显存容量；仅在 VSM 启用时生效。"), 128.f, 8192.f, 128.f, true},
		{TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectional"), TEXT("ShadowQuality"), 0.f, TEXT("VSM 远镜头精度 / 方向光 Bias"), TEXT("相机拉远仍需清晰时降低此值，例如 -1 到 -2；每降低 1 提高一级精度，同时增加页池与绘制成本。不是地面影子长度。"), -4.f, 4.f, 0.1f, false},
		{TEXT("r.Shadow.DistanceScale"), TEXT("ShadowQuality"), 1.f, TEXT("阴影距离倍率"), TEXT("缩放方向光 CSM 有效距离；更远会增加传统阴影绘制成本。"), 0.1f, 2.f, 0.05f, false},
		{TEXT("r.ViewDistanceScale"), TEXT("ViewDistanceQuality"), 1.f, TEXT("模型视距倍率"), TEXT("影响物体可见距离；提高会增加可见几何与绘制成本。"), 0.1f, 2.f, 0.05f, false},
		{TEXT("r.StaticMeshLODDistanceScale"), TEXT("ViewDistanceQuality"), 1.f, TEXT("静态模型 LOD 距离"), TEXT("提高会更早切换低 LOD，降低几何成本；降低则保留较高细节更久。"), 0.25f, 4.f, 0.05f, false},
		{TEXT("r.SkeletalMeshLODBias"), TEXT("ViewDistanceQuality"), 0.f, TEXT("骨骼模型 LOD Bias"), TEXT("提高 Bias 会更早选择低细节 LOD，降低角色几何成本。"), 0.f, 4.f, 1.f, true},
		{TEXT("r.Streaming.MipBias"), TEXT("TextureQuality"), 0.f, TEXT("纹理 Mip Bias"), TEXT("提高会更早使用低 mip，降低显存/带宽但可能变糊。"), 0.f, 4.f, 0.1f, false},
		{TEXT("r.MaxAnisotropy"), TEXT("TextureQuality"), 4.f, TEXT("最大各向异性"), TEXT("改善斜角纹理清晰度；较高值增加纹理采样成本。"), 1.f, 16.f, 1.f, true},
		{TEXT("r.MaterialQualityLevel"), TEXT("EffectsQuality"), 1.f, TEXT("材质质量等级"), TEXT("影响材质 Quality Switch 分支；更改后部分材质可能需要重编 Shader。"), 0.f, 3.f, 1.f, true},
		{TEXT("r.Shadow.Virtual.ResolutionLodBiasDirectionalMoving"), TEXT("ShadowQuality"), 0.f, TEXT("VSM 移动方向光 Bias"), TEXT("移动太阳/方向光的精度；每降低 1，线性分辨率翻倍，页需求约增至四倍。"), -4.f, 4.f, .1f, false},
		{TEXT("r.Shadow.Virtual.ResolutionLodBiasLocal"), TEXT("ShadowQuality"), 0.f, TEXT("VSM 局部灯分辨率 Bias"), TEXT("点光/聚光的分辨率偏移；负值提高远镜头细节，也增加 GPU 与页池压力。"), -4.f, 4.f, .1f, false},
		{TEXT("r.Shadow.Virtual.ResolutionLodBiasLocalMoving"), TEXT("ShadowQuality"), 0.f, TEXT("VSM 移动局部灯 Bias"), TEXT("移动局部灯的精度；不改变灯光 Source Radius 决定的柔影宽度。"), -4.f, 4.f, .1f, false},
		{TEXT("r.Shadow.Virtual.Clipmap.FirstLevel"), TEXT("ShadowQuality"), 6.f, TEXT("VSM 最近 Clipmap 层级"), TEXT("降低可提供更靠近相机的高精度层；远俯视镜头主要调分辨率 Bias，而非此项。"), 0.f, 16.f, 1.f, true},
		{TEXT("r.Shadow.Virtual.Clipmap.LastLevel"), TEXT("ShadowQuality"), 22.f, TEXT("VSM 最远 Clipmap 层级"), TEXT("方向光覆盖范围：每增加 1，最大范围翻倍；不是传统 SM 的距离倍率。"), 6.f, 22.f, 1.f, true},
		{TEXT("r.Shadow.Virtual.SMRT.RayCountDirectional"), TEXT("ShadowQuality"), 7.f, TEXT("方向光柔影射线数"), TEXT("提高减少半影噪点，增加 GPU 成本；0 为硬阴影，不提高贴图分辨率。"), 0.f, 16.f, 1.f, true},
		{TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayDirectional"), TEXT("ShadowQuality"), 8.f, TEXT("方向光每射线采样数"), TEXT("提高改善柔影采样；柔影宽度仍由灯光 Source Angle 控制。"), 1.f, 16.f, 1.f, true},
		{TEXT("r.Shadow.Virtual.SMRT.RayCountLocal"), TEXT("ShadowQuality"), 7.f, TEXT("局部灯柔影射线数"), TEXT("减少点光/聚光半影噪点；更多射线更昂贵，0 为硬阴影。"), 0.f, 16.f, 1.f, true},
		{TEXT("r.Shadow.Virtual.SMRT.SamplesPerRayLocal"), TEXT("ShadowQuality"), 8.f, TEXT("局部灯每射线采样数"), TEXT("改善半影采样；不能替代局部灯分辨率 Bias 或改变 Source Radius。"), 1.f, 16.f, 1.f, true},
		{TEXT("r.Lumen.ScreenProbeGather.DownsampleFactor"), TEXT("GlobalIlluminationQuality"), 16.f, TEXT("Lumen 屏幕探针间距 / 像素"), TEXT("越低探针越密，间接光空间细节更高、GPU 成本更大；后处理质量仍会参与最终值。"), 4.f, 64.f, 1.f, true},
		{TEXT("r.Lumen.ScreenProbeGather.TracingOctahedronResolution"), TEXT("GlobalIlluminationQuality"), 8.f, TEXT("Lumen 每探针射线分辨率"), TEXT("越高角度采样越细，射线数约按平方增加；仅 Lumen GI 启用时生效。"), 4.f, 16.f, 1.f, true},
		{TEXT("r.Lumen.Reflections.DownsampleFactor"), TEXT("ReflectionQuality"), 1.f, TEXT("Lumen 反射降采样"), TEXT("1 比 2 清晰，追踪成本更高；最终分辨率还受关卡后处理 Reflection Quality 影响。"), 1.f, 2.f, 1.f, true},
		{TEXT("r.Shadow.Virtual.Character.PreserveCasters"), TEXT("ShadowQuality"), 1.f, TEXT("保留角色远距离投影 / 0关闭 1开启"), TEXT("默认开启；保护 StylizedCharacterLit 非 Nanite 模型，豁免 VSM 远距离、屏幕半径和粗页小物件剔除。不强制最高 LOD，不取消视锥与光源范围管理。"), 0.f, 1.f, 1.f, true},
		{TEXT("r.Shadow.Virtual.NonNanite.UseRadiusThreshold"), TEXT("ShadowQuality"), 1.f, TEXT("非 Nanite 小投影剔除 / 0关闭 1开启"), TEXT("场景小物件的屏幕半径剔除开关。角色保护开启时角色豁免；全局关闭会增加远景投射物成本。"), 0.f, 1.f, 1.f, true},
		{TEXT("r.Shadow.RadiusThreshold"), TEXT("ShadowQuality"), .01f, TEXT("小投影屏幕半径阈值"), TEXT("不是世界距离；降低会保留更小的投射物、增加阴影绘制。角色保护开启时角色不受此阈值剔除。"), 0.f, .1f, .001f, false},
		{TEXT("r.Shadow.Virtual.CoarsePagePixelThresholdDynamic"), TEXT("ShadowQuality"), 16.f, TEXT("动态物件粗页剔除像素阈值"), TEXT("越低保留越多远处小物件；仅粗页规则。角色保护开启时角色豁免此项。"), 0.f, 64.f, 1.f, false},
		{TEXT("r.Shadow.Virtual.Character.ReceiverOffsetCm"), TEXT("ShadowQuality"), 0.f, TEXT("角色接收 / 自投影偏移 · cm"), TEXT("沿角色表面法线移动阴影采样位置；默认 0，建议先试 ±0.1。同时影响角色接收的外部投影，不改变地面影子。不是真正独立自阴影；过大导致漏光，负值可能产生痤疮。"), -2.f, 2.f, .01f, false},
	};

	const TCHAR* TierNames[QualityCount] = {TEXT("低 / Low"), TEXT("中 / Medium"), TEXT("高 / High"), TEXT("Epic")};
	IConsoleVariable* FindCVar(const TCHAR* Name)
	{
		// These are engine-owned variables with editor-process lifetime.
		// Slate text attributes run every frame: avoid repeated manager lookups.
		static TMap<FName, IConsoleVariable*> Cache;
		const FName Key(Name);
		if (IConsoleVariable** Existing = Cache.Find(Key)) return *Existing;
		IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
		if (Variable) Cache.Add(Key, Variable);
		return Variable;
	}

	float ReadDefaultValue(const FParameterDefinition& Definition, int32 Tier)
	{
		// Missing group entries use the constructor value, never a previous live override.
		float Value = Definition.ConstructorFallback;
		if (IConsoleVariable* Variable = FindCVar(Definition.CVar))
			Value = FCString::Atof(*Variable->GetDefaultValue());
		const FString Section = FString::Printf(TEXT("%s@%d"), Definition.ScalabilityGroup, Tier);
		float ConfigValue = 0.f;
		if (GConfig && GConfig->GetFloat(*Section, Definition.CVar, ConfigValue, GScalabilityIni)
			&& FMath::IsFinite(ConfigValue))
			Value = ConfigValue;
		return FMath::IsFinite(Value) ? Value : Definition.ConstructorFallback;
	}

	FString SerializePreset(const FDevKitQualityPreset& Preset)
	{
		FString Serialized = Preset.Name + TEXT("|");
		for (int32 Index = 0; Index < Preset.Values.Num(); ++Index)
		{
			if (Index > 0) Serialized += TEXT(",");
			Serialized += FString::SanitizeFloat(Preset.Values[Index]);
		}
		return Serialized;
	}

	bool DeserializePreset(const FString& Serialized, FDevKitQualityPreset& OutPreset)
	{
		FString ValuesString;
		if (!Serialized.Split(TEXT("|"), &OutPreset.Name, &ValuesString) || OutPreset.Name.IsEmpty()) return false;
		TArray<FString> Tokens;
		ValuesString.ParseIntoArray(Tokens, TEXT(","), true);
		const bool bLegacy = Tokens.Num() == 10 * QualityCount;
		if (!bLegacy && Tokens.Num() != 22 * QualityCount && Tokens.Num() != 26 * QualityCount && Tokens.Num() != ParameterCount * QualityCount) return false;
		OutPreset.Values.SetNum(ParameterCount * QualityCount);
		for (int32 Row = 0; Row < ParameterCount; ++Row)
			for (int32 Tier = 0; Tier < QualityCount; ++Tier)
			{
				const int32 Index = Row * QualityCount + Tier;
				float Value = ReadDefaultValue(ParameterDefinitions[Row], Tier);
				// Do not reactivate ignored shadow slots from an old saved matrix.
				if (Tokens.IsValidIndex(Index) && !(bLegacy && Row >= 1 && Row <= 3))
					if (!LexTryParseString(Value, *Tokens[Index]) || !FMath::IsFinite(Value)) return false;
				OutPreset.Values[Index] = FMath::Clamp(Value, ParameterDefinitions[Row].Min, ParameterDefinitions[Row].Max);
			}
		return true;
	}

	FText FormatQualityLevel(int32 Value)
	{
		return Value == INDEX_NONE ? FText::FromString(TEXT("未注册")) : FText::AsNumber(Value);
	}
}

void SDevKitGraphicsQualityWorkbenchWidget::Construct(const FArguments& InArgs)
{
	CaptureOpeningState();
	BuildDefaultValues();
	LoadPresets();
	ChildSlot
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Background"))).Padding(10.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot().Padding(8.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("画质分级与性能预设")))
					.Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("当前选中档位的修改立即预览；其他列是预设草稿，点击档位即可切换。选择预设或 Default 也会立即预览，Apply 可重新应用当前档。预设保存在本机编辑器设置，不提交项目配置。")))
					.AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DevKitWorkbenchUI::MakeSection(FText::FromString(TEXT("当前编辑器分级")),
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
						[
							SNew(STextBlock).Text(this, &SDevKitGraphicsQualityWorkbenchWidget::GetCurrentTierText).AutoWrapText(true).ColorAndOpacity(FStyleColors::AccentBlue)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(this, &SDevKitGraphicsQualityWorkbenchWidget::GetQualityBreakdownText).AutoWrapText(true)
						]
					)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					DevKitWorkbenchUI::MakeSection(FText::FromString(TEXT("预设 / Default / Apply")), SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)[SAssignNew(PresetCombo, SComboBox<TSharedPtr<FString>>).OptionsSource(&PresetOptions).OnGenerateWidget(this, &SDevKitGraphicsQualityWorkbenchWidget::MakePresetOption).OnSelectionChanged(this, &SDevKitGraphicsQualityWorkbenchWidget::OnPresetSelected)[SNew(STextBlock).Text_Lambda([this]() { return SelectedPreset.IsValid() ? FText::FromString(*SelectedPreset) : FText::FromString(TEXT("选择预设")); })]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SAssignNew(PresetNameBox, SEditableTextBox).HintText(FText::FromString(TEXT("预设名称"))).MinDesiredWidth(160.f)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(FText::FromString(TEXT("保存为预设"))).OnClicked(this, &SDevKitGraphicsQualityWorkbenchWidget::SavePreset)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).Text(FText::FromString(TEXT("Default"))).OnClicked(this, &SDevKitGraphicsQualityWorkbenchWidget::RestoreDefaults)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2.f)[SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("PrimaryButton")).Text(FText::FromString(TEXT("Apply"))).OnClicked(this, &SDevKitGraphicsQualityWorkbenchWidget::ApplySelectedTier)])
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SUniformGridPanel).SlotPadding(FMargin(6.f, 0.f))
					+ SUniformGridPanel::Slot(0, 0)[MakeTierButton(0, TierNames[0])]
					+ SUniformGridPanel::Slot(1, 0)[MakeTierButton(1, TierNames[1])]
					+ SUniformGridPanel::Slot(2, 0)[MakeTierButton(2, TierNames[2])]
					+ SUniformGridPanel::Slot(3, 0)[MakeTierButton(3, TierNames[3])]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					MakeParameterMatrix()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					MakeLightingDiagnostics()
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton")).HAlign(HAlign_Left)
					.Text(FText::FromString(TEXT("恢复本次打开面板前状态"))).OnClicked(this, &SDevKitGraphicsQualityWorkbenchWidget::RestoreProjectDefault)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock).Text(this, &SDevKitGraphicsQualityWorkbenchWidget::GetRenderingPathText).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock).Text(this, &SDevKitGraphicsQualityWorkbenchWidget::GetLastActionText).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		]
	];
}

void SDevKitGraphicsQualityWorkbenchWidget::CaptureOpeningState()
{
	OpeningQualityLevels = Scalability::GetQualityLevels();
	const int32 OpeningTier = OpeningQualityLevels.GetSingleQualityLevel();
	SelectedTier = OpeningTier >= 0 ? FMath::Clamp(OpeningTier, 0, QualityCount - 1) : 1;
	OpeningCVars.Reset(ParameterCount);
	for (int32 Row = 0; Row < ParameterCount; ++Row)
	{
		if (IsRetiredShadowRow(Row)) continue;
		const FParameterDefinition& Definition = ParameterDefinitions[Row];
		if (IConsoleVariable* Variable = FindCVar(Definition.CVar))
		{
			FDevKitQualityCVarSnapshot& Snapshot = OpeningCVars.AddDefaulted_GetRef();
			Snapshot.Name = Definition.CVar;
			Snapshot.Value = Variable->GetString();
			Snapshot.Flags = Variable->GetFlags();
		}
	}
}

TSharedRef<SWidget> SDevKitGraphicsQualityWorkbenchWidget::MakeParameterMatrix()
{
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	TSharedRef<SGridPanel> Header = SNew(SGridPanel);
	Header->SetColumnFill(0, 2.0f);
	for (int32 Column = 1; Column <= QualityCount; ++Column) Header->SetColumnFill(Column, 1.0f);
	Header->AddSlot(0, 0).Padding(8.f)[SNew(STextBlock).Text(FText::FromString(TEXT("参数 / 性能影响"))).Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))];
	for (int32 Tier = 0; Tier < QualityCount; ++Tier)
	{
		Header->AddSlot(Tier + 1, 0).Padding(8.f)[SNew(STextBlock).Text(FText::FromString(TierNames[Tier])).Justification(ETextJustify::Center)];
	}
	Rows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)[Header];
	const auto AddGroup = [this, &Rows](const TCHAR* Title, std::initializer_list<int32> Indices)
	{
		const TSharedRef<SVerticalBox> Group = SNew(SVerticalBox);
		for (int32 Row : Indices)
			Group->AddSlot().AutoHeight().Padding(0.f, 2.f)[MakeParameterRow(Row)];
		Rows->AddSlot().AutoHeight().Padding(0.f, 3.f)
			[DevKitWorkbenchUI::MakeSection(FText::FromString(Title), Group)];
	};
	AddGroup(TEXT("Lumen / GI 与反射精度"), {0, 19, 20, 21});
	AddGroup(TEXT("VSM / 相机距离与分辨率"), {2, 10, 11, 12, 13, 14, 1});
	AddGroup(TEXT("VSM / 角色投影保留与接收位置"), {22, 26, 23, 24, 25});
	AddGroup(TEXT("VSM / 柔影采样"), {15, 16, 17, 18});
	AddGroup(TEXT("模型 / 视距与 LOD"), {4, 5, 6});
	AddGroup(TEXT("纹理 / 材质质量"), {7, 8, 9});
	return SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel"))).Padding(8.f)[Rows];
}

TSharedRef<SWidget> SDevKitGraphicsQualityWorkbenchWidget::MakeParameterRow(const int32 RowIndex)
{
	const FParameterDefinition& Definition = ParameterDefinitions[RowIndex];
	TSharedRef<SGridPanel> Row = SNew(SGridPanel);
	Row->SetColumnFill(0, 2.0f);
	for (int32 Column = 1; Column <= QualityCount; ++Column) Row->SetColumnFill(Column, 1.0f);
	Row->AddSlot(0, 0).Padding(8.f).VAlign(VAlign_Center)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Definition.Label)).ToolTipText(FText::FromString(Definition.CVar))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 4.f, 0.f)[SNew(STextBlock).Text(FText::FromString(Definition.Impact)).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)[SNew(STextBlock).ColorAndOpacity(FStyleColors::AccentBlue)
			.Text_Lambda([&Definition]() { const auto* Variable = FindCVar(Definition.CVar); return FText::FromString(Variable ? FString::Printf(TEXT("实际值：%s"), *Variable->GetString()) : TEXT("当前引擎未注册，不能应用")); })]
	];
	for (int32 Tier = 0; Tier < QualityCount; ++Tier)
	{
		const int32 ValueIndex = RowIndex * QualityCount + Tier;
		Row->AddSlot(Tier + 1, 0).Padding(5.f).VAlign(VAlign_Center)
		[
			SNew(SBox).MinDesiredWidth(90.f)
			[
				SNew(SSpinBox<float>)
				.IsEnabled(FindCVar(Definition.CVar) != nullptr)
				.MinValue(Definition.Min).MaxValue(Definition.Max).Delta(Definition.Delta)
				.MinSliderValue(Definition.Min).MaxSliderValue(Definition.Max)
				.Value_Lambda([this, ValueIndex]() { return CurrentValues.IsValidIndex(ValueIndex) ? CurrentValues[ValueIndex] : 0.f; })
				.OnValueChanged_Lambda([this, RowIndex, Tier](const float Value)
				{
					EditParameter(RowIndex, Tier, Value);
				})
			]
		];
	}
	return SNew(SBorder).BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed"))).Padding(2.f)[Row];
}

TSharedRef<SWidget> SDevKitGraphicsQualityWorkbenchWidget::MakeTierButton(const int32 Quality, const TCHAR* Label)
{
	return SNew(SBorder).BorderImage_Lambda([this, Quality]() { return FAppStyle::GetBrush(SelectedTier == Quality ? TEXT("Brushes.Primary") : TEXT("Brushes.Recessed")); }).Padding(2.f)
	[
		SNew(SButton).ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton")).ContentPadding(FMargin(18.f, 10.f)).HAlign(HAlign_Center)
		.ForegroundColor_Lambda([this, Quality]() { return SelectedTier == Quality ? FStyleColors::White : FStyleColors::Foreground; })
		.Text(FText::FromString(Label)).OnClicked_Lambda([this, Quality]() { return SelectTier(Quality); })
	];
}

FReply SDevKitGraphicsQualityWorkbenchWidget::SelectTier(const int32 Quality)
{
	SelectedTier = FMath::Clamp(Quality, 0, QualityCount - 1);
	return ApplySelectedTier();
}

bool SDevKitGraphicsQualityWorkbenchWidget::PreviewParameter(int32 Row)
{
	if (IsRetiredShadowRow(Row)) return true;
	const FParameterDefinition& Definition = ParameterDefinitions[Row];
	IConsoleVariable* Variable = FindCVar(Definition.CVar);
	if (!Variable) return false;
	const float Value = CurrentValues[Row * QualityCount + SelectedTier];
	if (!FMath::IsFinite(Value)) return false;
	if (Row == 13 || Row == 14)
	{
		// Validate against the live counterpart, not only the draft pair: an
		// author console override may have prevented that endpoint from moving.
		const IConsoleVariable* Counterpart = FindCVar(ParameterDefinitions[Row == 13 ? 14 : 13].CVar);
		if (!Counterpart || (Row == 13 ? Value > Counterpart->GetFloat() : Value < Counterpart->GetFloat())) return false;
	}
	// An explicit editor preview must outrank project/device-profile ini values,
	// but must not silently overwrite command-line or console author overrides.
	if ((Variable->GetFlags() & ECVF_SetByMask) == ECVF_SetByCommandline)
		return FMath::IsNearlyEqual(Variable->GetFloat(), Value);
	if (Definition.bInteger) Variable->Set(FMath::RoundToInt(Value), ECVF_SetByCode);
	else Variable->Set(Value, ECVF_SetByCode);
	return FMath::IsNearlyEqual(Variable->GetFloat(), Value);
}

void SDevKitGraphicsQualityWorkbenchWidget::EditParameter(int32 Row, int32 Tier, float Value)
{
	const int32 Index = Row * QualityCount + Tier;
	if (!CurrentValues.IsValidIndex(Index) || !FMath::IsFinite(Value)) return;
	const FParameterDefinition& Definition = ParameterDefinitions[Row];
	Value = FMath::Clamp(Value, Definition.Min, Definition.Max);
	CurrentValues[Index] = Definition.bInteger ? static_cast<float>(FMath::RoundToInt(Value)) : Value;
	if (Tier != SelectedTier)
	{
		LastAction = FString::Printf(TEXT("已编辑 %s 草稿；当前预览为 %s。点击该档位即可切换预览。"), TierNames[Tier], TierNames[SelectedTier]);
		return;
	}
	if (CurrentValues[14 * QualityCount + Tier] < CurrentValues[13 * QualityCount + Tier])
	{
		LastAction = TEXT("未应用：VSM 最远 Clipmap 层级必须大于或等于最近层级。");
		return;
	}
	bool bApplied = false;
	if (Row == 13 || Row == 14)
	{
		// A previously invalid pair may have left either draft unapplied.
		// Once valid, preview both endpoints, expanding the live range first.
		const IConsoleVariable* LastLevel = FindCVar(ParameterDefinitions[14].CVar);
		const bool bExpandLastFirst = LastLevel && LastLevel->GetFloat() < CurrentValues[13 * QualityCount + Tier];
		const bool bFirstApplied = PreviewParameter(bExpandLastFirst ? 14 : 13);
		const bool bSecondApplied = PreviewParameter(bExpandLastFirst ? 13 : 14);
		bApplied = bFirstApplied && bSecondApplied;
	}
	else bApplied = PreviewParameter(Row);
	if (GEditor) GEditor->RedrawLevelEditingViewports();
	LastAction = FString::Printf(TEXT("%s：%s；实际值 %s。当前档实时预览，其他列为草稿；未写入项目配置。"),
		Definition.Label, bApplied ? TEXT("已实时应用") : TEXT("未生效：未注册或被命令行/控制台覆盖"),
		FindCVar(Definition.CVar) ? *FindCVar(Definition.CVar)->GetString() : TEXT("未注册"));
}

FReply SDevKitGraphicsQualityWorkbenchWidget::ApplySelectedTier()
{
	if (CurrentValues[14 * QualityCount + SelectedTier] < CurrentValues[13 * QualityCount + SelectedTier])
	{
		LastAction = TEXT("未应用：VSM 最远 Clipmap 层级必须大于或等于最近层级。");
		return FReply::Handled();
	}
	Scalability::FQualityLevels Levels = Scalability::GetQualityLevels();
	Levels.SetFromSingleQualityLevel(SelectedTier);
	// Force refresh also when the sg.* levels match but underlying values were customized.
	Scalability::SetQualityLevels(Levels, true);
	LastAppliedValues.Reset(ParameterCount);
	int32 UnappliedRows = 0;
	for (int32 Row = 0; Row < ParameterCount; ++Row)
	{
		if (IsRetiredShadowRow(Row))
		{
			LastAppliedValues.Add(FindCVar(ParameterDefinitions[Row].CVar) ? FindCVar(ParameterDefinitions[Row].CVar)->GetFloat() : 0.f);
			continue;
		}
		if (IConsoleVariable* Variable = FindCVar(ParameterDefinitions[Row].CVar))
		{
			const float Draft = CurrentValues[Row * QualityCount + SelectedTier];
			const float Value = FMath::Clamp(FMath::IsFinite(Draft) ? Draft : ReadDefaultValue(ParameterDefinitions[Row], SelectedTier), ParameterDefinitions[Row].Min, ParameterDefinitions[Row].Max);
			CurrentValues[Row * QualityCount + SelectedTier] = ParameterDefinitions[Row].bInteger ? static_cast<float>(FMath::RoundToInt(Value)) : Value;
			if (Row == 13)
			{
				const IConsoleVariable* LastLevel = FindCVar(ParameterDefinitions[14].CVar);
				if (LastLevel && LastLevel->GetFloat() < CurrentValues[13 * QualityCount + SelectedTier])
				{
					float& LastDraft = CurrentValues[14 * QualityCount + SelectedTier];
					LastDraft = FMath::Clamp(FMath::IsFinite(LastDraft) ? LastDraft : ReadDefaultValue(ParameterDefinitions[14], SelectedTier), ParameterDefinitions[14].Min, ParameterDefinitions[14].Max);
					LastDraft = static_cast<float>(FMath::RoundToInt(LastDraft));
					PreviewParameter(14);
				}
			}
			PreviewParameter(Row);
			const float Expected = ParameterDefinitions[Row].bInteger ? static_cast<float>(FMath::RoundToInt(Value)) : Value;
			LastAppliedValues.Add(Variable->GetFloat());
			if (!FMath::IsNearlyEqual(Variable->GetFloat(), Expected)) ++UnappliedRows;
		}
		else
		{
			LastAppliedValues.Add(0.f);
			++UnappliedRows;
		}
	}
	if (GEditor) GEditor->RedrawLevelEditingViewports();
	LastAction = FString::Printf(TEXT("已实时预览 %s，回读 %d 项参数；%d 项未按草稿生效（未注册或被命令行/控制台覆盖）。当前列修改立即生效，其他列为草稿。固定 VSM；GI / 反射遵循项目分级。"), TierNames[SelectedTier], ParameterCount - 1, UnappliedRows);
	return FReply::Handled();
}

FReply SDevKitGraphicsQualityWorkbenchWidget::RestoreDefaults()
{
	BuildDefaultValues();
	for (const TSharedPtr<FString>& Option : PresetOptions)
		if (Option.IsValid() && *Option == TEXT("Default")) { SelectedPreset = Option; break; }
	return ApplySelectedTier();
}

void SDevKitGraphicsQualityWorkbenchWidget::BuildDefaultValues()
{
	CurrentValues.SetNum(ParameterCount * QualityCount);
	for (int32 Row = 0; Row < ParameterCount; ++Row)
		for (int32 Tier = 0; Tier < QualityCount; ++Tier)
			CurrentValues[Row * QualityCount + Tier] = ReadDefaultValue(ParameterDefinitions[Row], Tier);
}

FReply SDevKitGraphicsQualityWorkbenchWidget::SavePreset()
{
	const FString Name = PresetNameBox.IsValid() ? PresetNameBox->GetText().ToString().TrimStartAndEnd() : FString();
	if (Name.IsEmpty())
	{
		LastAction = TEXT("请先填写预设名称。");
		return FReply::Handled();
	}
	if (Name.Equals(TEXT("Default"), ESearchCase::IgnoreCase))
	{
		LastAction = TEXT("Default 保留为项目 Scalability 默认值；请使用其他名称保存自定义预设。");
		return FReply::Handled();
	}
	FDevKitQualityPreset* Existing = Presets.FindByPredicate([&Name](const FDevKitQualityPreset& Item) { return Item.Name.Equals(Name, ESearchCase::IgnoreCase); });
	if (!Existing) Existing = &Presets.AddDefaulted_GetRef();
	Existing->Name = Name;
	Existing->Values = CurrentValues;
	SavePresets();
	LoadPresets();
	LoadPreset(Name);
	if (PresetCombo.IsValid()) PresetCombo->RefreshOptions();
	LastAction = FString::Printf(TEXT("预设“%s”已保存到本机项目编辑器设置；不会修改项目配置文件。"), *Name);
	return FReply::Handled();
}

void SDevKitGraphicsQualityWorkbenchWidget::LoadPresets()
{
	Presets.Reset();
	TArray<FString> SerializedPresets;
	if (GConfig) GConfig->GetArray(PresetSection, PresetKey, SerializedPresets, GEditorPerProjectIni);
	for (const FString& Serialized : SerializedPresets)
	{
		FDevKitQualityPreset Preset;
		if (DeserializePreset(Serialized, Preset)) Presets.Add(MoveTemp(Preset));
	}
	FDevKitQualityPreset* DefaultPreset = Presets.FindByPredicate([](const FDevKitQualityPreset& Item) { return Item.Name.Equals(TEXT("Default"), ESearchCase::IgnoreCase); });
	if (!DefaultPreset)
	{
		DefaultPreset = &Presets.AddDefaulted_GetRef();
	}
	// The built-in Default is derived from the merged config, not a stale local preset.
	DefaultPreset->Name = TEXT("Default");
	DefaultPreset->Values.SetNum(ParameterCount * QualityCount);
	for (int32 Row = 0; Row < ParameterCount; ++Row)
		for (int32 Tier = 0; Tier < QualityCount; ++Tier)
			DefaultPreset->Values[Row * QualityCount + Tier] = ReadDefaultValue(ParameterDefinitions[Row], Tier);
	PresetOptions.Reset();
	for (const FDevKitQualityPreset& Preset : Presets) PresetOptions.Add(MakeShared<FString>(Preset.Name));
	if (PresetOptions.Num() > 0 && !SelectedPreset.IsValid())
	{
		SelectedPreset = PresetOptions[0];
		LoadPreset(*SelectedPreset);
	}
}

void SDevKitGraphicsQualityWorkbenchWidget::SavePresets() const
{
	if (!GConfig) return;
	TArray<FString> SerializedPresets;
	for (const FDevKitQualityPreset& Preset : Presets) SerializedPresets.Add(SerializePreset(Preset));
	GConfig->SetArray(PresetSection, PresetKey, SerializedPresets, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

void SDevKitGraphicsQualityWorkbenchWidget::LoadPreset(const FString& PresetName)
{
	if (const FDevKitQualityPreset* Preset = Presets.FindByPredicate([&PresetName](const FDevKitQualityPreset& Item) { return Item.Name == PresetName; }))
	{
		if (Preset->Values.Num() == ParameterCount * QualityCount) CurrentValues = Preset->Values;
		for (const TSharedPtr<FString>& Option : PresetOptions)
			if (Option.IsValid() && *Option == PresetName) { SelectedPreset = Option; break; }
		LastAction = FString::Printf(TEXT("已载入预设“%s”的四档参数。当前档位用于实时预览，其他列保留为草稿。"), *PresetName);
	}
}

void SDevKitGraphicsQualityWorkbenchWidget::OnPresetSelected(TSharedPtr<FString> Preset, ESelectInfo::Type SelectInfo)
{
	if (Preset.IsValid()) { LoadPreset(*Preset); ApplySelectedTier(); }
}

TSharedRef<SWidget> SDevKitGraphicsQualityWorkbenchWidget::MakePresetOption(TSharedPtr<FString> Preset) const
{
	return SNew(STextBlock).Text(Preset.IsValid() ? FText::FromString(*Preset) : FText::GetEmpty()).Margin(FMargin(6.f));
}

FReply SDevKitGraphicsQualityWorkbenchWidget::RestoreProjectDefault()
{
	Scalability::SetQualityLevels(OpeningQualityLevels, true);
	for (const FDevKitQualityCVarSnapshot& Snapshot : OpeningCVars)
	{
		if (IConsoleVariable* Variable = FindCVar(*Snapshot.Name))
		{
			Variable->ClearFlags(ECVF_SetByMask);
			Variable->Set(*Snapshot.Value, static_cast<EConsoleVariableFlags>(Snapshot.Flags & ECVF_SetByMask));
			Variable->SetFlags(Snapshot.Flags);
		}
	}
	if (GEditor) GEditor->RedrawLevelEditingViewports();
	LastAction = TEXT("已恢复本次打开面板前的 Scalability、26 项参数与优先级；不写入项目配置，草稿和预设保留，传统 SM 距离不应用。");
	return FReply::Handled();
}

FText SDevKitGraphicsQualityWorkbenchWidget::GetCurrentTierText() const
{
	const int32 Quality = Scalability::GetQualityLevels().GetSingleQualityLevel();
	return Quality == INDEX_NONE ? FText::FromString(TEXT("自定义 / 混合分级")) : GetTierName(Quality);
}

FText SDevKitGraphicsQualityWorkbenchWidget::GetQualityBreakdownText() const
{
	const Scalability::FQualityLevels Q = Scalability::GetQualityLevels();
	return FText::FromString(FString::Printf(
		TEXT("分辨率 %.0f%%  ·  视距 %s  ·  抗锯齿 %s  ·  阴影 %s  ·  GI %s  ·  反射 %s  ·  后处理 %s  ·  纹理 %s  ·  特效 %s  ·  植被 %s  ·  着色 %s  ·  地形 %s"),
		Q.ResolutionQuality, *FormatQualityLevel(Q.ViewDistanceQuality).ToString(), *FormatQualityLevel(Q.AntiAliasingQuality).ToString(),
		*FormatQualityLevel(Q.ShadowQuality).ToString(), *FormatQualityLevel(Q.GlobalIlluminationQuality).ToString(),
		*FormatQualityLevel(Q.ReflectionQuality).ToString(), *FormatQualityLevel(Q.PostProcessQuality).ToString(),
		*FormatQualityLevel(Q.TextureQuality).ToString(), *FormatQualityLevel(Q.EffectsQuality).ToString(),
		*FormatQualityLevel(Q.FoliageQuality).ToString(), *FormatQualityLevel(Q.ShadingQuality).ToString(),
		*FormatQualityLevel(Q.LandscapeQuality).ToString()));
}

FText SDevKitGraphicsQualityWorkbenchWidget::GetRenderingPathText() const
{
	auto Read = [](const TCHAR* Name) { const IConsoleVariable* Variable = FindCVar(Name); return Variable ? Variable->GetInt() : INDEX_NONE; };
	const int32 GIMethod = Read(TEXT("r.DynamicGlobalIlluminationMethod"));
	const int32 GIAllowed = Read(TEXT("r.Lumen.DiffuseIndirect.Allow"));
	const int32 ReflectionMethod = Read(TEXT("r.ReflectionMethod"));
	const int32 ReflectionsAllowed = Read(TEXT("r.Lumen.Reflections.Allow"));
	const int32 VirtualShadows = Read(TEXT("r.Shadow.Virtual.Enable"));
	const FString GI = GIMethod == INDEX_NONE || GIAllowed == INDEX_NONE ? TEXT("未知") : (GIMethod == 1 && GIAllowed != 0 ? TEXT("Lumen 已启用") : TEXT("非 Lumen / 已关闭"));
	const FString Reflections = ReflectionMethod == INDEX_NONE || ReflectionsAllowed == INDEX_NONE ? TEXT("未知") : (ReflectionMethod == 1 && ReflectionsAllowed != 0 ? TEXT("Lumen 已启用") : TEXT("非 Lumen / 已关闭"));
	const FString Shadows = VirtualShadows == INDEX_NONE ? TEXT("未知") : (UseVirtualShadowMaps(GMaxRHIShaderPlatform) ? TEXT("VSM 平台路径已启用")
		: (VirtualShadows != 0 ? TEXT("请求 VSM，但平台/项目支持未满足（需检查配置并重启）") : TEXT("VSM 未启用")));
	return FText::FromString(FString::Printf(TEXT("实时渲染路径：全局照明 %s；反射 %s；阴影 %s。工具固定 VSM，不提供 SM 切换。负分辨率 Bias 提高同一相机距离的精度；页容量只防止溢出，不直接提高精度。"), *GI, *Reflections, *Shadows));
}

TSharedRef<SWidget> SDevKitGraphicsQualityWorkbenchWidget::MakeLightingDiagnostics()
{
	const TSharedRef<SWrapBox> Buttons = SNew(SWrapBox).UseAllottedSize(true);
	const auto Add = [this, &Buttons](const TCHAR* Label, EViewModeIndex Mode, const TCHAR* Name = TEXT(""))
	{
		Buttons->AddSlot().Padding(3.f)[SNew(SButton).Text(FText::FromString(Label))
			.OnClicked_Lambda([this, Mode, Name]()
			{
				FText Message;
				FDevKitLookDevLevelService::SetPreviewViewMode(Mode, FName(Name), Message);
				LastAction = Message.ToString();
				return FReply::Handled();
			})];
	};
	Add(TEXT("恢复 Lit"), VMI_Lit);
	Add(TEXT("VSM 遮罩"), VMI_VisualizeVirtualShadowMap, TEXT("mask"));
	Add(TEXT("VSM 层级 / Mip"), VMI_VisualizeVirtualShadowMap, TEXT("mip"));
	Add(TEXT("VSM 页缓存"), VMI_VisualizeVirtualShadowMap, TEXT("cache"));
	Add(TEXT("Lumen 总览"), VMI_VisualizeLumen, TEXT("Overview"));
	Add(TEXT("Lumen Scene"), VMI_VisualizeLumen, TEXT("LumenScene"));
	Add(TEXT("Lumen Surface Cache"), VMI_VisualizeLumen, TEXT("SurfaceCache"));
	Add(TEXT("光照复杂度"), VMI_LightComplexity);
	Add(TEXT("仅光照"), VMI_LightingOnly);
	return DevKitWorkbenchUI::MakeSection(FText::FromString(TEXT("UE 原生光影诊断 / 当前关卡视口")),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Buttons]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)[SNew(STextBlock).AutoWrapText(true)
			.Text(FText::FromString(TEXT("只切视口模式，不保存资产或改曝光。VSM Mip 用于检查远镜头分辨率；柔影宽度来自灯光 Source Angle / Radius。Lumen 关闭时调精度不产生 GI；关卡后处理也会影响最终质量。")))]);
}

FText SDevKitGraphicsQualityWorkbenchWidget::GetLastActionText() const
{
	return FText::FromString(LastAction);
}

FText SDevKitGraphicsQualityWorkbenchWidget::GetTierName(const int32 Quality)
{
	return Quality >= 0 && Quality < QualityCount ? FText::FromString(TierNames[Quality]) : FText::FromString(TEXT("自定义 / 混合分级"));
}
