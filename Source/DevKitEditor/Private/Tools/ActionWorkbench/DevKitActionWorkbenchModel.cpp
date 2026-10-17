#include "Tools/ActionWorkbench/DevKitActionWorkbenchModel.h"

#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "ScopedTransaction.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "DevKitActionWorkbenchModel"

namespace DevKitActionWorkbench
{
	bool IsPlaySessionActive()
	{
		if (!GEditor) return false;
		if (GEditor->PlayWorld || GEditor->bIsSimulatingInEditor) return true;
		for (const FWorldContext& Context : GEditor->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE) return true;
		}
		return false;
	}

	const FStateConflictRule* FindRule(const UStateConflictDataAsset* Asset, FGameplayTag Active, bool& bDuplicate)
	{
		const FStateConflictRule* Result = nullptr;
		if (Asset)
		{
			for (const FStateConflictRule& Rule : Asset->Rules)
			{
				if (Rule.ActiveTag == Active)
				{
					bDuplicate |= Result != nullptr;
					Result = &Rule; // Runtime builds a map; the last duplicate wins. UI refuses editing it.
				}
			}
		}
		return Result;
	}

	bool Matches(const FGameplayTagContainer& Targets, FGameplayTag Identity, bool& bInherited)
	{
		bool bMatches = false;
		for (const FGameplayTag& Target : Targets)
		{
			if (Identity.MatchesTag(Target))
			{
				bMatches = true;
				bInherited |= Target != Identity;
			}
		}
		return bMatches;
	}

	TArray<TSharedPtr<FJsonValue>> TagValues(const FGameplayTagContainer& Tags)
	{
		TArray<FString> Names;
		for (const FGameplayTag& Tag : Tags) Names.Add(Tag.ToString());
		Names.Sort();
		TArray<TSharedPtr<FJsonValue>> Result;
		for (const FString& Name : Names) Result.Add(MakeShared<FJsonValueString>(Name));
		return Result;
	}
}

bool FDevKitActionWorkbenchModel::Load(UStateConflictDataAsset* Asset, FText& OutMessage)
{
	if (!IsValid(Asset))
	{
		OutMessage = LOCTEXT("InvalidSource", "没有有效的 StateConflict 资产；现有草稿未改变。");
		return false;
	}
	if (Source.Get() == Asset && DiskFingerprint(Asset) != BaselineDiskFingerprint)
	{
		OutMessage = LOCTEXT("DiskReloadRequired", "磁盘文件已变化，但编辑器仍持有同一源对象。请先在内容浏览器真正 Reload 资产或重启编辑器，不能仅重建草稿绕过版本保护。");
		return false;
	}
	// Keep a freshly soft-loaded source alive across allocations. The draft owns only the two
	// authorable policy fields; do not duplicate a loaded package's UObject/subobject graph.
	TStrongObjectPtr<UStateConflictDataAsset> SourceGuard(Asset);
	UStateConflictDataAsset* NewDraft = NewObject<UStateConflictDataAsset>(GetTransientPackage(), NAME_None, RF_Transient | RF_Transactional);
	if (!NewDraft)
	{
		OutMessage = LOCTEXT("DuplicateFailed", "无法创建独立草稿；源资产没有改变。");
		return false;
	}
	NewDraft->ClearFlags(RF_Public | RF_Standalone);
	NewDraft->SetFlags(RF_Transient | RF_Transactional);
	NewDraft->Rules = Asset->Rules;
	NewDraft->BlockCategoryMap = Asset->BlockCategoryMap;
	Source.Reset(Asset);
	Draft.Reset(NewDraft);
	bOwnsDirtySource = false;
	CaptureBaseline();
	OutMessage = LOCTEXT("Loaded", "已读取真实资产并创建独立草稿；尚未改变游戏规则。");
	return true;
}

bool FDevKitActionWorkbenchModel::Reset(FText& OutMessage)
{
	return Load(Source.Get(), OutMessage);
}

void FDevKitActionWorkbenchModel::CaptureBaseline()
{
	Baseline = Snapshot(Source.Get());
	BaselineDiskFingerprint = DiskFingerprint(Source.Get());
	bSourceWasDirty = Source.IsValid() && Source->GetOutermost()->IsDirty();
}

FString FDevKitActionWorkbenchModel::DiskFingerprint(const UStateConflictDataAsset* Asset)
{
	if (!Asset) return TEXT("no_source");
	FString Filename;
	if (!FPackageName::DoesPackageExist(Asset->GetOutermost()->GetName(), &Filename)) return TEXT("no_disk_package");
	const FMD5Hash Hash = FMD5Hash::HashFile(*Filename);
	return Hash.IsValid() ? Filename + TEXT(":") + LexToString(Hash) : TEXT("unreadable_package");
}

bool FDevKitActionWorkbenchModel::IsDirty() const
{
	return Draft.IsValid() && Snapshot(Draft.Get()) != Baseline;
}

bool FDevKitActionWorkbenchModel::HasSourceChanged() const
{
	return !Source.IsValid() || Snapshot(Source.Get()) != Baseline
		|| DiskFingerprint(Source.Get()) != BaselineDiskFingerprint;
}

TArray<FDevKitActionWorkbenchIssue> FDevKitActionWorkbenchModel::Validate() const
{
	TArray<FDevKitActionWorkbenchIssue> Issues = ValidateAsset(Draft.Get());
	if (HasSourceChanged())
	{
		Issues.Add({true, LOCTEXT("Stale", "源资产或磁盘文件已变化。请先导出草稿，再重载和重新核对，不能覆盖新版本。"), INDEX_NONE});
	}
	return Issues;
}

TArray<FDevKitActionWorkbenchIssue> FDevKitActionWorkbenchModel::ValidateAsset(const UStateConflictDataAsset* Asset)
{
	TArray<FDevKitActionWorkbenchIssue> Issues;
	if (!Asset)
	{
		Issues.Add({true, LOCTEXT("NoDraft", "没有可校验的规则资产。"), INDEX_NONE});
		return Issues;
	}
	TSet<FGameplayTag> Seen;
	for (int32 Index = 0; Index < Asset->Rules.Num(); ++Index)
	{
		const FStateConflictRule& Rule = Asset->Rules[Index];
		if (!Rule.ActiveTag.IsValid())
		{
			Issues.Add({true, FText::Format(LOCTEXT("EmptyActive", "规则 {0} 没有有效的触发状态。"), Index + 1), Index});
		}
		else if (Seen.Contains(Rule.ActiveTag))
		{
			Issues.Add({true, FText::Format(LOCTEXT("DuplicateActive", "触发状态 {0} 重复。运行时只保留最后一条，必须先消除歧义。"), FText::FromName(Rule.ActiveTag.GetTagName())), Index});
		}
		Seen.Add(Rule.ActiveTag);
		if (Rule.Priority != 0)
		{
			Issues.Add({false, LOCTEXT("UnusedPriority", "Priority 目前不参与 StateConflict 准入；不能用这个数值代表动作优先级。"), Index});
		}
		for (const FGameplayTagContainer* Targets : {&Rule.BlockTags, &Rule.CancelTags})
		{
			for (const FGameplayTag& Tag : *Targets)
			{
				if (!Tag.IsValid()) Issues.Add({true, LOCTEXT("InvalidTarget", "阻断/取消列表包含无效标签。"), Index});
			}
		}
		bool bInherited = false;
		if (DevKitActionWorkbench::Matches(Rule.CancelTags, Rule.ActiveTag, bInherited))
		{
			Issues.Add({false, LOCTEXT("SelfCancel", "此状态的取消目标也匹配自身身份；请在真实能力上核对是否会自取消。"), Index});
		}
	}
	for (const auto& Pair : Asset->BlockCategoryMap)
	{
		if (!Pair.Key.IsValid())
		{
			Issues.Add({true, LOCTEXT("InvalidCategory", "资源分类包含无效的分类标签。"), INDEX_NONE});
		}
		else if (Pair.Key.ToString() != TEXT("Block.Movement") && Pair.Key.ToString() != TEXT("Block.AI"))
		{
			Issues.Add({false, FText::Format(LOCTEXT("UnknownCategory", "分类 {0} 不属于已确认的 Movement/AI 适配器；仅保存配置不代表运行时已经支持。"), FText::FromName(Pair.Key.GetTagName())), INDEX_NONE});
		}
		for (const FGameplayTag& State : Pair.Value)
		{
			if (!State.IsValid()) Issues.Add({true, LOCTEXT("InvalidCategoryState", "资源分类包含无效的触发状态。"), INDEX_NONE});
		}
	}
	Issues.Add({false, LOCTEXT("PartialValidation", "校验仅覆盖本资产。GA/蓝图、输入、成本、冷却、窗口、对话和旧入口仍须独立回归；本表未限制不代表允许执行。"), INDEX_NONE});
	return Issues;
}

bool FDevKitActionWorkbenchModel::ApplyDraft(FText& OutMessage)
{
	if (DevKitActionWorkbench::IsPlaySessionActive())
	{
		OutMessage = LOCTEXT("PIE", "PIE / Simulate 期间不能应用规则；请先停止运行。不会热切角色冲突表。");
		return false;
	}
	for (const FDevKitActionWorkbenchIssue& Issue : Validate())
	{
		if (Issue.bError) { OutMessage = Issue.Message; return false; }
	}
	if (BaselineDiskFingerprint == TEXT("unreadable_package"))
	{
		OutMessage = LOCTEXT("UnreadableSource", "无法读取源资产磁盘文件，不能建立可靠基线；已拒绝应用。");
		return false;
	}
	if (bOwnsDirtySource)
	{
		OutMessage = LOCTEXT("SavePending", "上次应用尚未完成保存确认。请先显式保存该资产或撤销应用，再重新载入草稿。");
		return false;
	}
	if (bSourceWasDirty || (Source->GetOutermost()->IsDirty() && !bOwnsDirtySource))
	{
		OutMessage = LOCTEXT("DirtySource", "源资产已含其他未保存修改。请在原编辑器处理并保存/撤销，再重载工作台；不会夹带保存。");
		return false;
	}
	FString Filename;
	if (FPackageName::DoesPackageExist(Source->GetOutermost()->GetName(), &Filename)
		&& IFileManager::Get().IsReadOnly(*Filename))
	{
		OutMessage = LOCTEXT("ReadOnly", "源文件为只读。请在项目的源控制流程确认所有者并签出，再应用；工作台不会自动签出或提交。");
		return false;
	}
	if (!IsDirty())
	{
		OutMessage = LOCTEXT("Unchanged", "没有需要应用的草稿差异。");
		return true;
	}
	const FScopedTransaction Transaction(LOCTEXT("ApplyTransaction", "应用动作工作台规则草稿"));
	Source->SetFlags(RF_Transactional);
	Source->Modify();
	Source->Rules = Draft->Rules;
	Source->BlockCategoryMap = Draft->BlockCategoryMap;
	Source->PostEditChange();
	Source->MarkPackageDirty();
	Baseline = Snapshot(Source.Get());
	bOwnsDirtySource = true;
	OutMessage = LOCTEXT("Applied", "已将草稿应用为可撤销的资产修改，尚未保存到磁盘。新 PIE 初始化才读取规则；没有修改运行中的角色。");
	return true;
}

bool FDevKitActionWorkbenchModel::AcknowledgeSavedSource(FText& OutMessage)
{
	if (!bOwnsDirtySource || !Source.IsValid() || Source->GetOutermost()->IsDirty() || Snapshot(Source.Get()) != Baseline)
	{
		OutMessage = LOCTEXT("SaveNotConfirmed", "没有可确认的本工作台保存，或源数据在应用后再次变化；请保留草稿并重新核对。");
		return false;
	}
	const FString Fingerprint = DiskFingerprint(Source.Get());
	if (Fingerprint == TEXT("unreadable_package") || Fingerprint == TEXT("no_disk_package"))
	{
		OutMessage = LOCTEXT("NoSavedFile", "未找到可读的已保存资产文件，不能确认落盘。");
		return false;
	}
	CaptureBaseline();
	bOwnsDirtySource = false;
	OutMessage = LOCTEXT("SaveConfirmed", "已确认资产保存并更新草稿基线；没有提交到源控制或发布。");
	return true;
}

bool FDevKitActionWorkbenchModel::SetBlocked(FGameplayTag Request, FGameplayTag Current, bool bEnabled, FText& OutMessage)
{
	return SetTarget(Current, Request, false, bEnabled, OutMessage);
}

bool FDevKitActionWorkbenchModel::SetCancel(FGameplayTag Request, FGameplayTag Current, bool bEnabled, FText& OutMessage)
{
	return SetTarget(Request, Current, true, bEnabled, OutMessage);
}

bool FDevKitActionWorkbenchModel::SetTarget(FGameplayTag Active, FGameplayTag Target, bool bCancel, bool bEnabled, FText& OutMessage)
{
	if (DevKitActionWorkbench::IsPlaySessionActive())
	{
		OutMessage = LOCTEXT("DraftPIE", "PIE / Simulate 期间只允许查看；请先停止运行再编辑草稿。");
		return false;
	}
	if (!Draft.IsValid() || !Active.IsValid() || !Target.IsValid())
	{
		OutMessage = LOCTEXT("InvalidPair", "没有有效草稿或动作标签。不会创建新 GameplayTag。");
		return false;
	}
	bool bDuplicate = false;
	const FStateConflictRule* Existing = DevKitActionWorkbench::FindRule(Draft.Get(), Active, bDuplicate);
	if (bDuplicate)
	{
		OutMessage = LOCTEXT("DuplicateEdit", "同一触发状态存在重复规则，请先在高级数据中解决；没有修改草稿。");
		return false;
	}
	if (Existing && !bEnabled)
	{
		bool bInherited = false;
		DevKitActionWorkbench::Matches(bCancel ? Existing->CancelTags : Existing->BlockTags, Target, bInherited);
		if (bInherited)
		{
			OutMessage = LOCTEXT("InheritedEdit", "当前关系受父标签覆盖。不能只为这一格删除共享父标签；请在高级数据中核对所有受影响动作后显式修改。");
			return false;
		}
	}
	if ((!Existing && !bEnabled) || (Existing && ((bCancel ? Existing->CancelTags : Existing->BlockTags).HasTagExact(Target) == bEnabled)))
	{
		OutMessage = LOCTEXT("NoTargetChange", "这条精确目标配置已经是所选状态；没有修改草稿。");
		return true;
	}
	const FScopedTransaction Transaction(LOCTEXT("EditDraftTransaction", "修改动作工作台规则草稿"));
	Draft->Modify();
	FStateConflictRule* Rule = Draft->Rules.FindByPredicate([Active](const FStateConflictRule& Entry) { return Entry.ActiveTag == Active; });
	if (!Rule)
	{
		Rule = &Draft->Rules.AddDefaulted_GetRef();
		Rule->ActiveTag = Active;
	}
	FGameplayTagContainer& Tags = bCancel ? Rule->CancelTags : Rule->BlockTags;
	if (bEnabled) Tags.AddTag(Target);
	else Tags.RemoveTag(Target);
	OutMessage = LOCTEXT("DraftChanged", "已修改独立草稿；源资产和当前游戏尚未改变。");
	return true;
}

TArray<FDevKitActionWorkbenchAction> FDevKitActionWorkbenchModel::GetActions(const UStateConflictDataAsset* Asset)
{
	TArray<FDevKitActionWorkbenchAction> Actions;
	TSet<FGameplayTag> Added;
	const auto Add = [&Actions, &Added](FGameplayTag Tag, FText Name, bool bBuiltin)
	{
		if (Tag.IsValid() && !Added.Contains(Tag))
		{
			Added.Add(Tag);
			Actions.Add({Tag, Name, bBuiltin});
		}
	};
	Add(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack"), false), LOCTEXT("Attack", "普攻"), true);
	Add(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash"), false), LOCTEXT("Dash", "冲刺"), true);
	Add(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill"), false), LOCTEXT("WeaponSkill", "战技"), true);
	Add(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Active"), false), LOCTEXT("ActiveSkill", "主动技能"), true);
	if (Asset)
	{
		TArray<FGameplayTag> Inventory;
		for (const FStateConflictRule& Rule : Asset->Rules)
		{
			Inventory.AddUnique(Rule.ActiveTag);
			for (const FGameplayTag& Tag : Rule.BlockTags) Inventory.AddUnique(Tag);
			for (const FGameplayTag& Tag : Rule.CancelTags) Inventory.AddUnique(Tag);
		}
		for (const auto& Pair : Asset->BlockCategoryMap)
		{
			for (const FGameplayTag& Tag : Pair.Value) Inventory.AddUnique(Tag);
		}
		Inventory.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });
		for (const FGameplayTag& Tag : Inventory) Add(Tag, FText::FromName(Tag.GetTagName()), false);
	}
	return Actions;
}

FDevKitActionWorkbenchCell FDevKitActionWorkbenchModel::EvaluateCell(const UStateConflictDataAsset* Asset, FGameplayTag Request, FGameplayTag Current)
{
	FDevKitActionWorkbenchCell Result;
	if (!Asset || !Request.IsValid() || !Current.IsValid())
	{
		Result.bAmbiguous = true;
		Result.Explanation = LOCTEXT("NoCell", "没有有效的资产/标签，不能推导动作许可。");
		return Result;
	}
	const FStateConflictRule* CurrentRule = DevKitActionWorkbench::FindRule(Asset, Current, Result.bAmbiguous);
	const FStateConflictRule* RequestRule = DevKitActionWorkbench::FindRule(Asset, Request, Result.bAmbiguous);
	Result.bHasCurrentRule = CurrentRule != nullptr;
	Result.bHasRequestRule = RequestRule != nullptr;
	if (CurrentRule) Result.bBlocked = DevKitActionWorkbench::Matches(CurrentRule->BlockTags, Request, Result.bInheritedBlock);
	if (RequestRule) Result.bCancels = DevKitActionWorkbench::Matches(RequestRule->CancelTags, Current, Result.bInheritedCancel);
	if (Result.bAmbiguous)
	{
		Result.Explanation = LOCTEXT("AmbiguousCell", "触发状态存在重复规则。以下标记仅展示最后一条数据；先消除歧义，不能据此判定真实动作结果。");
	}
	else if (Result.bBlocked && Result.bCancels)
	{
		Result.Explanation = LOCTEXT("BlockBeforeCancel", "当前状态阻止请求身份。同时配置了新状态出现后的取消目标，但取消不是准入豁免：新动作被阻止时，不能依靠自身尚未出现的状态解除阻断。");
	}
	else if (Result.bBlocked)
	{
		Result.Explanation = LOCTEXT("BlockedCell", "当前状态的 BlockTags 匹配请求身份：本表对该身份设有阻断。其他 GA 标签/旧别名和输入限制仍需真实角色诊断。");
	}
	else if (Result.bCancels)
	{
		Result.Explanation = LOCTEXT("CancelCell", "请求状态出现后，其 CancelTags 匹配当前身份。只是取消目标声明，不保证提前取消、准入成功、目标可取消或窗口合法。");
	}
	else
	{
		Result.Explanation = LOCTEXT("NoRestriction", "本表未命中这对身份的阻断/取消，不等于允许或共存。GA/蓝图、额外 AbilityTags、输入、成本、冷却和窗口可能继续限制。");
	}
	return Result;
}

FString FDevKitActionWorkbenchModel::Snapshot(const UStateConflictDataAsset* Asset)
{
	if (!Asset) return TEXT("null");
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("schemaVersion"), 1);
	Root->SetStringField(TEXT("kind"), TEXT("Dev01.StateConflictDraft"));
	TArray<TSharedPtr<FJsonValue>> Rules;
	for (const FStateConflictRule& Rule : Asset->Rules)
	{
		TSharedRef<FJsonObject> JsonRule = MakeShared<FJsonObject>();
		JsonRule->SetStringField(TEXT("activeTag"), Rule.ActiveTag.ToString());
		JsonRule->SetArrayField(TEXT("blockTags"), DevKitActionWorkbench::TagValues(Rule.BlockTags));
		JsonRule->SetArrayField(TEXT("cancelTags"), DevKitActionWorkbench::TagValues(Rule.CancelTags));
		JsonRule->SetNumberField(TEXT("priority"), Rule.Priority);
		Rules.Add(MakeShared<FJsonValueObject>(JsonRule));
	}
	Root->SetArrayField(TEXT("rules"), Rules);
	TArray<FGameplayTag> Keys;
	Asset->BlockCategoryMap.GetKeys(Keys);
	Keys.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });
	TArray<TSharedPtr<FJsonValue>> Categories;
	for (const FGameplayTag& Key : Keys)
	{
		TSharedRef<FJsonObject> Category = MakeShared<FJsonObject>();
		Category->SetStringField(TEXT("category"), Key.ToString());
		Category->SetArrayField(TEXT("states"), DevKitActionWorkbench::TagValues(Asset->BlockCategoryMap.FindChecked(Key)));
		Categories.Add(MakeShared<FJsonValueObject>(Category));
	}
	Root->SetArrayField(TEXT("categories"), Categories);
	FString Result;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Result);
	FJsonSerializer::Serialize(Root, Writer);
	return Result;
}

#undef LOCTEXT_NAMESPACE
