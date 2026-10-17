#include "Tools/ActionWorkbench/SDevKitActionRuleSourcePanel.h"

#include "ActionRules/DevKitActionRuleSubsystem.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitActionRuleSourcePanel"

namespace ActionRuleSourceUI
{
	TSharedRef<STextBlock> Text(const FText& Value, int32 Size = 10, FSlateColor Color = FStyleColors::Foreground)
	{
		return SNew(STextBlock).Text(Value).AutoWrapText(true).ColorAndOpacity(Color)
			.Font(FCoreStyle::GetDefaultFontStyle(Size >= 14 ? "Bold" : "Regular", Size));
	}
	FString Root() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("ActionRules/")); }
	bool ParseTags(const FString& Input, FGameplayTagContainer& Out, FString& Error)
	{
		TArray<FString> Names;
		Input.ParseIntoArray(Names, TEXT(","), true);
		for (FString Name : Names)
		{
			Name.TrimStartAndEndInline();
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Name), false);
			if (!Tag.IsValid()) { Error = TEXT("未注册的 Tag: ") + Name; return false; }
			Out.AddTag(Tag);
		}
		return true;
	}
}

void FDevKitActionRuleSourceDraft::SetProfile(const FString& InProfile)
{
	if (Profile != InProfile) { Profile = InProfile; Candidate.Reset(); }
}

bool FDevKitActionRuleSourceDraft::Validate(FString& Error)
{
	Candidate.Reset();
	FString FullPath;
	if (!UDevKitActionRuleSubsystem::ResolveProfilePath(Profile, FullPath, Error)) return false;
	Candidate = FDevKitActionRuleSet::LoadFile(FullPath, Error);
	return Candidate.IsValid();
}

bool FDevKitActionRuleSourceDraft::RevalidateForApply(FString& Error)
{
	if (!Candidate) { Error = TEXT("请先校验候选规则。"); return false; }
	FString FullPath;
	if (!UDevKitActionRuleSubsystem::ResolveProfilePath(Profile, FullPath, Error)) { Candidate.Reset(); return false; }
	const auto Fresh = FDevKitActionRuleSet::LoadFile(FullPath, Error);
	if (!Fresh) { Candidate.Reset(); return false; }
	if (Fresh->Revision != Candidate->Revision || Fresh->SourceFile != Candidate->SourceFile)
	{
		Candidate.Reset();
		Error = TEXT("JSON 或 Lua 在校验后已改变；未保存配置，请重新校验并检查规则。");
		return false;
	}
	Candidate = Fresh;
	Error.Reset();
	return true;
}

bool SDevKitActionRuleSourcePanel::IsPlaying()
{
	if (!GEditor) return false;
	if (GEditor->PlayWorld || GEditor->bIsSimulatingInEditor) return true;
	for (const FWorldContext& Context : GEditor->GetWorldContexts())
		if (Context.WorldType == EWorldType::PIE) return true;
	return false;
}

void SDevKitActionRuleSourcePanel::Construct(const FArguments& InArgs)
{
	const auto* Settings = GetDefault<UDevKitActionRuleSettings>();
	Draft.SetProfile(Settings->Profile);
	bDraftEnabled = Settings->bEnabled;
	Status = LOCTEXT("Initial", "选择候选 → 只读校验 → 明确保存挂载配置。打开面板不会执行脚本或启用规则。");
	Evaluation = LOCTEXT("NoSimulation", "候选模拟不会激活动作、修改 Tag 或解除旧锁。");
	WorldStatus = LOCTEXT("NoWorldStatus", "点击刷新读取开发世界状态；不会自动重载正在运行的规则。");
	ChildSlot
	[
		SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FStyleColors::Panel).Padding(20)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ActionRuleSourceUI::Text(LOCTEXT("Eyebrow", "AUTHORING SOURCE  /  OPTIONAL MOUNT"), 9, FStyleColors::AccentBlue)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 8)[ActionRuleSourceUI::Text(LOCTEXT("Heading", "表格与脚本，轻量管理动作关系"), 20)]
				+ SVerticalBox::Slot().AutoHeight()[ActionRuleSourceUI::Text(LOCTEXT("Lead", "Excel / CSV 经 Python 转换为规则包；Lua 描述条件。仅列出有规则的关系，不再逐格维护完整矩阵。"), 10, FStyleColors::ForegroundHeader)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 8)
				[SNew(STextBlock).Text_Lambda([]
				{
					const auto* S = GetDefault<UDevKitActionRuleSettings>();
					return FText::FromString(FString::Printf(TEXT("项目配置：%s    |    %s    |    下次 PIE / 新世界生效"), S->bEnabled ? TEXT("已启用") : TEXT("未启用 · 保留既有行为"), *S->Profile));
				}).AutoWrapText(true).ColorAndOpacity(FStyleColors::AccentBlue)]
				+ SVerticalBox::Slot().AutoHeight()[ActionRuleSourceUI::Text(LOCTEXT("RelativePath", "候选路径 · 相对 Content/ActionRules"), 10)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 10)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)
					[SNew(SEditableTextBox).Text_Lambda([this] { return FText::FromString(Draft.GetProfile()); })
						.HintText(LOCTEXT("ProfileHint", "MyCombat/rules.json"))
						.OnTextChanged_Lambda([this](const FText& V)
						{
							Draft.SetProfile(V.ToString()); Status = LOCTEXT("PathChanged", "路径已修改；候选校验已失效，配置尚未保存。");
							Evaluation = LOCTEXT("SimulationReset", "重新校验后才能模拟。"); RefreshRuleRows();
						})]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)[SNew(SButton).Text(LOCTEXT("Browse", "选择 JSON…")).OnClicked(this, &SDevKitActionRuleSourcePanel::BrowseProfile)]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10, 8))
					+ SWrapBox::Slot()[SNew(SButton).Text(LOCTEXT("Validate", "校验候选 · 不挂载")).OnClicked(this, &SDevKitActionRuleSourcePanel::ValidateCandidate)]
					+ SWrapBox::Slot()[SNew(SCheckBox).IsEnabled_Lambda([] { return !IsPlaying(); })
						.IsChecked_Lambda([this] { return bDraftEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bDraftEnabled = S == ECheckBoxState::Checked; Status = LOCTEXT("TogglePending", "启用状态仅为待保存选择；点击保存后才修改项目配置。"); })
						[ActionRuleSourceUI::Text(LOCTEXT("Enable", "启用外部规则（待保存）"))]]
					+ SWrapBox::Slot()[SNew(SButton).IsEnabled_Lambda([] { return !IsPlaying(); })
						.Text(LOCTEXT("SaveMount", "保存挂载配置 · 下次 PIE")).OnClicked(this, &SDevKitActionRuleSourcePanel::SaveMountSettings)]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 12)
				[SNew(STextBlock).Text_Lambda([this] { return Status; }).AutoWrapText(true).ColorAndOpacity(FStyleColors::ForegroundHeader)]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10, 6))
					+ SWrapBox::Slot()[SNew(SButton).Text(LOCTEXT("RuleFolder", "打开规则 / Lua 目录")).OnClicked_Lambda([this]
					{
						FString Path, Error;
						const FString Directory = UDevKitActionRuleSubsystem::ResolveProfilePath(Draft.GetProfile(), Path, Error) ? FPaths::GetPath(Path) : ActionRuleSourceUI::Root();
						FPlatformProcess::ExploreFolder(*Directory); return FReply::Handled();
					})]
					+ SWrapBox::Slot()[SNew(SButton).Text(LOCTEXT("PythonFolder", "打开 Python 转换工具 / 说明")).OnClicked_Lambda([]
					{ FPlatformProcess::ExploreFolder(*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Tools/ActionRules"))); return FReply::Handled(); })]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 22, 0, 8)[ActionRuleSourceUI::Text(LOCTEXT("SparseRules", "候选规则 · 仅显示配置的关系"), 14)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[ActionRuleSourceUI::Text(LOCTEXT("RuleDirection", "方向：当前状态 → 新请求；interrupt 表示新请求打断当前状态。"), 10, FStyleColors::ForegroundHeader)]
				+ SVerticalBox::Slot().AutoHeight()[SAssignNew(RuleRows, SBox)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 22, 0, 8)[ActionRuleSourceUI::Text(LOCTEXT("SimulationTitle", "只读模拟 · 候选快照"), 14)]
				+ SVerticalBox::Slot().AutoHeight()[ActionRuleSourceUI::Text(LOCTEXT("SimulationHint", "使用完整 Tag，以逗号分隔；运行主体为 player / enemy / other。表格中的 any 表示适用所有主体。数值可写 phase=1, stamina=80。不查询真实角色。"), 9, FStyleColors::ForegroundHeader)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)[ActionRuleSourceUI::Text(LOCTEXT("CurrentLabel", "当前状态 Tags"), 9)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
				[SNew(SEditableTextBox).Text_Lambda([this] { return FText::FromString(CurrentInput); }).HintText(LOCTEXT("Current", "当前状态 Tags"))
					.OnTextChanged_Lambda([this](const FText& V) { CurrentInput = V.ToString(); InvalidateSimulation(); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[ActionRuleSourceUI::Text(LOCTEXT("RequestLabel", "请求动作 AbilityTags"), 9)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
				[SNew(SEditableTextBox).Text_Lambda([this] { return FText::FromString(RequestInput); }).HintText(LOCTEXT("Request", "请求动作 AbilityTags"))
					.OnTextChanged_Lambda([this](const FText& V) { RequestInput = V.ToString(); InvalidateSimulation(); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(0.22f)[SNew(SEditableTextBox).Text_Lambda([this] { return FText::FromString(SubjectInput); })
						.OnTextChanged_Lambda([this](const FText& V) { SubjectInput = V.ToString(); InvalidateSimulation(); })]
					+ SHorizontalBox::Slot().FillWidth(0.78f).Padding(8, 0)[SNew(SEditableTextBox).Text_Lambda([this] { return FText::FromString(ValuesInput); })
						.HintText(LOCTEXT("Values", "数值上下文，可留空"))
						.OnTextChanged_Lambda([this](const FText& V) { ValuesInput = V.ToString(); InvalidateSimulation(); })]
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(LOCTEXT("Evaluate", "模拟判定")).OnClicked(this, &SDevKitActionRuleSourcePanel::EvaluateCandidate)]
				]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this] { return Evaluation; }).AutoWrapText(true)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 22, 0, 8)[SNew(SButton).Text(LOCTEXT("RefreshWorld", "刷新开发世界状态 · 只读")).OnClicked(this, &SDevKitActionRuleSourcePanel::RefreshWorldStatus)]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this] { return WorldStatus; }).AutoWrapText(true)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 0)[ActionRuleSourceUI::Text(LOCTEXT("Compatibility", "迁移边界：旧 StateConflict 资产、GA / 蓝图限制、冷却与费用、输入 / Montage 锁仍独立生效。候选未阻止 ≠ GAS 最终允许。旧矩阵保留用于兼容检查；本页不写 .uasset，不清空锁，不自动热重载。"), 10, FStyleColors::Warning)]
			]
		]
	];
	RefreshRuleRows();
}

FReply SDevKitActionRuleSourcePanel::BrowseProfile()
{
	TArray<FString> Files;
	if (IDesktopPlatform* Desktop = FDesktopPlatformModule::Get())
	{
		if (Desktop->OpenFileDialog(FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared()),
			TEXT("选择动作规则 JSON"), ActionRuleSourceUI::Root(), TEXT(""), TEXT("Action Rules (*.json)|*.json"), 0, Files) && !Files.IsEmpty())
		{
			FString Relative = Files[0];
			FPaths::MakePathRelativeTo(Relative, *ActionRuleSourceUI::Root());
			FString FullPath, Error;
			if (UDevKitActionRuleSubsystem::ResolveProfilePath(Relative, FullPath, Error))
			{
				Draft.SetProfile(Relative); Evaluation = FText::GetEmpty();
				Status = LOCTEXT("Selected", "候选已选择，尚未校验或挂载。"); RefreshRuleRows();
			}
			else Status = FText::FromString(Error);
		}
	}
	return FReply::Handled();
}

FReply SDevKitActionRuleSourcePanel::ValidateCandidate()
{
	FString Error;
	Status = Draft.Validate(Error)
		? LOCTEXT("Validated", "候选校验成功；配置和正在运行的世界均未改变。确认规则后再保存挂载配置。")
		: FText::FromString(TEXT("校验失败：") + Error);
	Evaluation = Draft.GetCandidate().IsValid()
		? LOCTEXT("NewCandidate", "可使用下方模拟检查本次校验快照；模拟不执行真实动作。")
		: LOCTEXT("InvalidCandidate", "校验失败，没有可模拟的候选。");
	RefreshRuleRows();
	return FReply::Handled();
}

FReply SDevKitActionRuleSourcePanel::SaveMountSettings()
{
	if (IsPlaying()) { Status = LOCTEXT("PlayDenied", "PIE / SIE 中禁止修改项目挂载配置。请先结束运行。"); return FReply::Handled(); }
	auto* Settings = GetMutableDefault<UDevKitActionRuleSettings>();
	FString Error;
	// A broken/missing existing profile must still be explicitly disableable.
	const bool bDisableOnly = !bDraftEnabled && Draft.GetProfile() == Settings->Profile;
	if (!bDisableOnly && !Draft.RevalidateForApply(Error))
	{
		Status = FText::FromString(Error); RefreshRuleRows(); return FReply::Handled();
	}
	const FString ConfigPath = Settings->GetDefaultConfigFilename();
	if (IFileManager::Get().IsReadOnly(*ConfigPath))
	{
		Status = LOCTEXT("ConfigReadOnly", "DefaultGame.ini 为只读；请自行签出该配置再保存。本页不会自动签出或创建 CL。"); return FReply::Handled();
	}
	const bool OldEnabled = Settings->bEnabled;
	const FString OldProfile = Settings->Profile;
	Settings->bEnabled = bDraftEnabled;
	Settings->Profile = Draft.GetProfile();
	if (!Settings->TryUpdateDefaultConfigFile())
	{
		Settings->bEnabled = OldEnabled; Settings->Profile = OldProfile;
		Status = LOCTEXT("SaveFailed", "挂载配置保存失败；内存配置已恢复，请检查文件权限。");
		return FReply::Handled();
	}
	// This engine's TryUpdateDefaultConfigFile only reports the read-only gate.
	// Verify persisted values independently before calling the mount saved.
	FConfigFile Persisted;
	Persisted.Read(ConfigPath);
	const TCHAR* Section = TEXT("/Script/DevKit.DevKitActionRuleSettings");
	bool PersistedEnabled = false;
	FString PersistedProfile;
	Persisted.GetBool(Section, TEXT("bEnabled"), PersistedEnabled);
	if (!Persisted.GetString(Section, TEXT("Profile"), PersistedProfile)
		|| PersistedEnabled != bDraftEnabled || PersistedProfile != Draft.GetProfile())
	{
		Settings->bEnabled = OldEnabled; Settings->Profile = OldProfile;
		Status = LOCTEXT("SaveUnverified", "未能核实 DefaultGame.ini 写入结果；磁盘可能已改变，未自动回滚。内存配置已恢复，请检查磁盘配置与权限，不要将本次视为已挂载。");
		return FReply::Handled();
	}
	Status = LOCTEXT("Saved", "已保存 Config/DefaultGame.ini。只影响下次 PIE / 新世界；没有修改规则资产或重载当前世界。");
	return FReply::Handled();
}

void SDevKitActionRuleSourcePanel::RefreshRuleRows()
{
	if (!RuleRows) return;
	const auto Candidate = Draft.GetCandidate();
	if (!Candidate)
	{
		RuleRows->SetContent(ActionRuleSourceUI::Text(LOCTEXT("NotValidated", "尚无已校验候选。规则源中的动作目录与关系将在这里展示。"), 10, FStyleColors::ForegroundHeader));
		return;
	}
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 10)
	[ActionRuleSourceUI::Text(FText::FromString(FString::Printf(TEXT("%s  ·  %d 动作 / %d 规则\nRevision  %s"), *Candidate->Id, Candidate->Actions.Num(), Candidate->Rules.Num(), *Candidate->Revision)), 10, FStyleColors::AccentBlue)];
	// Bound Slate work independently of the runtime profile's larger rule limit.
	constexpr int32 MaxPreviewRows = 200;
	int32 PreviewRows = 0;
	for (const auto& Rule : Candidate->Rules)
	{
		if (PreviewRows++ >= MaxPreviewRows) break;
		const FString Relation = FString::Printf(TEXT("%s   |   %s → %s   |   %s   |   %s%s"),
			*Rule.Id, *Rule.Current, *Rule.Request, *Rule.Subject, *Rule.Effect, Rule.bEnabled ? TEXT("") : TEXT(" [已停用]"));
		Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 4)
		[SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush")).BorderBackgroundColor(FStyleColors::Recessed).Padding(12)
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ActionRuleSourceUI::Text(FText::FromString(Relation), 10, Rule.Effect == TEXT("block") ? FStyleColors::AccentRed : FStyleColors::AccentBlue)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[ActionRuleSourceUI::Text(FText::FromString(Rule.Reason + (Rule.Condition.IsEmpty() ? FString() : TEXT("   ·   Lua: ") + Rule.Condition)), 9, FStyleColors::ForegroundHeader)]]];
	}
	if (Candidate->Rules.Num() > MaxPreviewRows)
		Rows->AddSlot().AutoHeight()[ActionRuleSourceUI::Text(FText::FromString(FString::Printf(TEXT("预览前 %d 条，共 %d 条；完整规则请在表格 / JSON 源中检索。模拟仍使用全部规则。"), MaxPreviewRows, Candidate->Rules.Num())), 9, FStyleColors::ForegroundHeader)];
	if (Candidate->Rules.IsEmpty()) Rows->AddSlot().AutoHeight()[ActionRuleSourceUI::Text(LOCTEXT("EmptyRules", "此候选未定义限制；既有 GAS / 资产限制仍保留。"))];
	RuleRows->SetContent(SNew(SBox).MaxDesiredHeight(330)[SNew(SScrollBox) + SScrollBox::Slot()[Rows]]);
}

void SDevKitActionRuleSourcePanel::InvalidateSimulation()
{
	Evaluation = LOCTEXT("InputsChanged", "模拟输入已改变，请重新点击模拟判定；上次结论已失效。");
}

FReply SDevKitActionRuleSourcePanel::EvaluateCandidate()
{
	const auto Candidate = Draft.GetCandidate();
	if (!Candidate) { Evaluation = LOCTEXT("ValidateFirst", "请先校验候选。"); return FReply::Handled(); }
	FGameplayTagContainer Current, Request;
	FString Error;
	if (!ActionRuleSourceUI::ParseTags(CurrentInput, Current, Error) || !ActionRuleSourceUI::ParseTags(RequestInput, Request, Error))
	{ Evaluation = FText::FromString(Error); return FReply::Handled(); }
	if (Request.IsEmpty() || (SubjectInput != TEXT("player") && SubjectInput != TEXT("enemy") && SubjectInput != TEXT("other")))
	{ Evaluation = LOCTEXT("BadContext", "请求 Tags 不可为空；运行主体须为 player / enemy / other，不能使用表格通配范围 any。"); return FReply::Handled(); }
	TMap<FString, double> Values;
	TArray<FString> Pairs; ValuesInput.ParseIntoArray(Pairs, TEXT(","), true);
	for (const FString& Pair : Pairs)
	{
		FString Key, Number; double Value = 0;
		if (!Pair.Split(TEXT("="), &Key, &Number) || !LexTryParseString(Value, *Number.TrimStartAndEnd()) || !FMath::IsFinite(Value))
		{ Evaluation = LOCTEXT("BadValue", "数值格式错误；示例 stamina=80, phase=1。"); return FReply::Handled(); }
		Key.TrimStartAndEndInline();
		if (Key.IsEmpty() || Values.Contains(Key)) { Evaluation = LOCTEXT("BadKey", "数值名称不可为空或重复。"); return FReply::Handled(); }
		Values.Add(Key, Value);
	}
	const auto Result = Candidate->Evaluate(Request, Current, SubjectInput, Values);
	Evaluation = FText::FromString(FString::Printf(TEXT("候选结果：%s%s\n命中规则：%s\n原因：%s\n进入后取消目标：%s\n此处仅计算候选，未执行 GAS 激活或取消。"),
		Result.bBlocked ? TEXT("阻止") : TEXT("本规则源未阻止"), Result.bError ? TEXT("（规则错误，拒绝放行）") : TEXT(""),
		*FString::Join(Result.MatchedRules, TEXT(", ")), *FString::Join(Result.Reasons, TEXT(" / ")), *Result.CancelAbilityTags.ToStringSimple()));
	return FReply::Handled();
}

FReply SDevKitActionRuleSourcePanel::RefreshWorldStatus()
{
	TArray<FString> Lines;
	if (GEditor)
		for (const FWorldContext& Context : GEditor->GetWorldContexts())
			if ((Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World())
				if (const auto* Service = Context.World()->GetSubsystem<UDevKitActionRuleSubsystem>())
					Lines.Add(Context.World()->GetName() + TEXT(": ") + Service->GetStatus());
	WorldStatus = Lines.IsEmpty() ? LOCTEXT("NoWorld", "未发现开发运行世界。启动隔离测试 PIE 后再刷新；本工具不会启动游戏或加载存档。") : FText::FromString(FString::Join(Lines, TEXT("\n")));
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
