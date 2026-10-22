#include "ActionRules/DevKitActionRuleSet.h"
#if WITH_DEVKIT_LUA_RULES
#include "DevKitLuaRuleVM.h"
#endif
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"

namespace
{
	constexpr int64 MaxSourceBytes = 1024 * 1024;
	bool StrictJson(const FString& Json, FString& Error)
	{
		struct FScope { bool bObject; TSet<FString> Keys; };
		TArray<FScope> Scopes;
		const auto Reader = TJsonReaderFactory<>::Create(Json);
		EJsonNotation Notation;
		while (Reader->ReadNext(Notation))
		{
			if (Notation == EJsonNotation::Error) { Error = Reader->GetErrorMessage(); return false; }
			if (Notation == EJsonNotation::ObjectEnd || Notation == EJsonNotation::ArrayEnd)
			{
				if (!Scopes.IsEmpty()) Scopes.Pop();
				continue;
			}
			if (!Scopes.IsEmpty() && Scopes.Last().bObject)
			{
				const FString Key = Reader->GetIdentifier();
				if (Scopes.Last().Keys.Contains(Key)) { Error = TEXT("Duplicate JSON key: ") + Key; return false; }
				Scopes.Last().Keys.Add(Key);
			}
			if (Notation == EJsonNotation::ObjectStart || Notation == EJsonNotation::ArrayStart)
			{
				if (Scopes.Num() >= 32) { Error = TEXT("JSON nesting exceeds 32 levels"); return false; }
				Scopes.Add({Notation == EJsonNotation::ObjectStart, {}});
			}
		}
		if (!Reader->GetErrorMessage().IsEmpty()) { Error = Reader->GetErrorMessage(); return false; }
		return true;
	}
	bool Fields(const TSharedPtr<FJsonObject>& O, const TArray<FString>& Allowed, FString& Error)
	{
		for (const auto& Pair : O->Values)
			if (!Allowed.Contains(FString(Pair.Key))) { Error = TEXT("Unknown field: ") + FString(Pair.Key); return false; }
		return true;
	}
	bool String(const TSharedPtr<FJsonObject>& O, const TCHAR* Key, FString& Out, FString& Error, bool Optional = false)
	{
		if (Optional && !O->HasField(Key)) { Out.Reset(); return true; }
		if (!O->HasTypedField<EJson::String>(Key) || !O->TryGetStringField(Key, Out) || (!Optional && Out.TrimStartAndEnd().IsEmpty()))
		{ Error = FString::Printf(TEXT("Expected non-empty string: %s"), Key); return false; }
		return true;
	}
	bool Tags(const TSharedPtr<FJsonObject>& O, const TCHAR* Key, FGameplayTagContainer& Out, FString& Error)
	{
		const TArray<TSharedPtr<FJsonValue>>* Items;
		if (!O->TryGetArrayField(Key, Items) || Items->IsEmpty()) { Error = FString(Key) + TEXT(" must be a non-empty tag array"); return false; }
		for (const auto& Item : *Items)
		{
			FString Name;
			if (Item->Type != EJson::String || !Item->TryGetString(Name)) { Error = TEXT("Tag names must be strings"); return false; }
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Name), false);
			if (!Tag.IsValid() || Tag.ToString() != Name || Out.HasTagExact(Tag))
			{ Error = TEXT("Unknown, redirected or duplicate tag: ") + Name; return false; }
			Out.AddTag(Tag);
		}
		return true;
	}
	bool Object(const TSharedPtr<FJsonValue>& Value, TSharedPtr<FJsonObject>& Out, FString& Error)
	{
		if (Value->Type != EJson::Object) { Error = TEXT("Expected a JSON object"); return false; }
		Out = Value->AsObject(); return true;
	}
}

TSharedPtr<FDevKitActionRuleSet> FDevKitActionRuleSet::LoadFile(const FString& Filename, FString& Error)
{
	Error.Reset();
	const int64 Size = IFileManager::Get().FileSize(*Filename);
	FString Json;
	if (Size < 0 || Size > MaxSourceBytes || !FFileHelper::LoadFileToString(Json, *Filename))
	{ Error = TEXT("Cannot read rule JSON (missing or over 1 MiB): ") + Filename; return nullptr; }
	TSharedPtr<FJsonObject> O;
	if (!StrictJson(Json, Error)) return nullptr;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), O) || !O.IsValid())
	{ Error = TEXT("Invalid JSON"); return nullptr; }
	FString LuaFile, Lua;
	if (!String(O, TEXT("luaFile"), LuaFile, Error, true)) return nullptr;
	if (!LuaFile.IsEmpty())
	{
		// A bundle owns a single adjacent script. No arbitrary filesystem mounts.
		if (FPaths::GetCleanFilename(LuaFile) != LuaFile || FPaths::GetExtension(LuaFile) != TEXT("lua") || LuaFile.Contains(TEXT(":")))
		{ Error = TEXT("luaFile must be an adjacent .lua filename"); return nullptr; }
		const FString LuaPath = FPaths::GetPath(Filename) / LuaFile;
		const int64 LuaSize = IFileManager::Get().FileSize(*LuaPath);
		if (LuaSize < 0 || LuaSize > MaxSourceBytes || !FFileHelper::LoadFileToString(Lua, *LuaPath))
		{ Error = TEXT("Cannot read Lua source: ") + LuaPath; return nullptr; }
	}
	auto Result = Parse(Json, Lua, Error);
	if (Result) Result->SourceFile = Filename;
	return Result;
}

TSharedPtr<FDevKitActionRuleSet> FDevKitActionRuleSet::Parse(const FString& Json, const FString& Lua, FString& Error)
{
	Error.Reset();
	if (FTCHARToUTF8(*Json).Length() > MaxSourceBytes || FTCHARToUTF8(*Lua).Length() > MaxSourceBytes) { Error = TEXT("Source too large"); return nullptr; }
	if (!StrictJson(Json, Error)) return nullptr;
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid())
	{ Error = TEXT("Invalid JSON"); return nullptr; }
	if (!Fields(Root, {TEXT("schemaVersion"), TEXT("id"), TEXT("luaFile"), TEXT("actions"), TEXT("rules")}, Error)) return nullptr;
	double Schema = 0;
	if (!Root->HasTypedField<EJson::Number>(TEXT("schemaVersion")) || !Root->TryGetNumberField(TEXT("schemaVersion"), Schema) || Schema != 1) { Error = TEXT("schemaVersion must be the number 1"); return nullptr; }
	auto Set = MakeShared<FDevKitActionRuleSet>();
	if (!String(Root, TEXT("id"), Set->Id, Error)) return nullptr;
	FString DeclaredLuaFile;
	if (!String(Root, TEXT("luaFile"), DeclaredLuaFile, Error, true)) return nullptr;
	if (!DeclaredLuaFile.IsEmpty() && Lua.IsEmpty()) { Error = TEXT("Declared Lua source is empty"); return nullptr; }
	const TArray<TSharedPtr<FJsonValue>>* Items;
	if (!Root->TryGetArrayField(TEXT("actions"), Items) || Items->IsEmpty() || Items->Num() > 1024)
	{ Error = TEXT("actions must contain 1..1024 entries"); return nullptr; }
	TSet<FString> ActionIds;
	for (const auto& Item : *Items)
	{
		TSharedPtr<FJsonObject> O; FDevKitActionDefinition A;
		if (!Object(Item, O, Error) || !Fields(O, {TEXT("id"),TEXT("displayName"),TEXT("abilityTags"),TEXT("stateTags")}, Error)
			|| !String(O, TEXT("id"), A.Id, Error) || !String(O,TEXT("displayName"),A.DisplayName,Error)
			|| !Tags(O,TEXT("abilityTags"),A.AbilityTags,Error) || !Tags(O,TEXT("stateTags"),A.StateTags,Error)) return nullptr;
		if (ActionIds.Contains(A.Id)) { Error = TEXT("Duplicate action: ") + A.Id; return nullptr; }
		for (const auto& Existing : Set->Actions)
			if (A.AbilityTags.HasAny(Existing.AbilityTags) || Existing.AbilityTags.HasAny(A.AbilityTags))
			{ Error = TEXT("Overlapping action abilityTags: ") + A.Id + TEXT(" / ") + Existing.Id; return nullptr; }
		ActionIds.Add(A.Id); Set->ActionIndices.Add(A.Id, Set->Actions.Num()); Set->Actions.Add(MoveTemp(A));
	}
	if (!Root->TryGetArrayField(TEXT("rules"), Items) || Items->Num() > 8192)
	{ Error = TEXT("rules must be an array of at most 8192 entries"); return nullptr; }
	TSet<FString> RuleIds;
	bool bUsesLua = false;
	for (const auto& Item : *Items)
	{
		TSharedPtr<FJsonObject> O; FDevKitActionRule R;
		if (!Object(Item, O, Error) || !Fields(O, {TEXT("id"),TEXT("enabled"),TEXT("subject"),TEXT("current"),TEXT("request"),TEXT("effect"),TEXT("condition"),TEXT("reason")}, Error)
			|| !String(O,TEXT("id"),R.Id,Error) || !String(O,TEXT("subject"),R.Subject,Error)
			|| !String(O,TEXT("current"),R.Current,Error) || !String(O,TEXT("request"),R.Request,Error)
			|| !String(O,TEXT("effect"),R.Effect,Error) || !String(O,TEXT("condition"),R.Condition,Error,true)
			|| !String(O,TEXT("reason"),R.Reason,Error)) return nullptr;
		if (!O->HasTypedField<EJson::Boolean>(TEXT("enabled")) || !O->TryGetBoolField(TEXT("enabled"), R.bEnabled)) { Error = TEXT("enabled must be an explicit boolean"); return nullptr; }
		if (RuleIds.Contains(R.Id)) { Error = TEXT("Duplicate rule: ") + R.Id; return nullptr; }
		if (!ActionIds.Contains(R.Current) || !ActionIds.Contains(R.Request)) { Error = TEXT("Unknown action in rule: ") + R.Id; return nullptr; }
		if (R.Subject != TEXT("any") && R.Subject != TEXT("player") && R.Subject != TEXT("enemy")) { Error = TEXT("Invalid subject: ") + R.Id; return nullptr; }
		if (R.Effect != TEXT("block") && R.Effect != TEXT("interrupt")) { Error = TEXT("Invalid effect: ") + R.Id; return nullptr; }
		if (R.Current == R.Request && R.Effect == TEXT("interrupt")) { Error = TEXT("Self interruption is not supported: ") + R.Id; return nullptr; }
		RuleIds.Add(R.Id); bUsesLua |= !R.Condition.IsEmpty();
		Set->RulesByRequest.FindOrAdd(R.Request).Add(Set->Rules.Num()); Set->Rules.Add(MoveTemp(R));
	}
	if (bUsesLua || !Lua.IsEmpty())
	{
#if WITH_DEVKIT_LUA_RULES
		Set->VM = MakeShared<FDevKitLuaRuleVM>();
		if (!Set->VM->LoadSource(Lua, Error)) return nullptr;
		// Validate callable names, without treating a sample invocation as semantic proof.
		for (const auto& Rule : Set->Rules)
			if (!Rule.Condition.IsEmpty() && !Set->VM->HasCondition(Rule.Condition))
			{ Error = TEXT("Missing Lua condition: ") + Rule.Condition; return nullptr; }
#else
		// Reject at load time so a Lua-dependent profile never runs with its conditions silently ignored.
		Error = TEXT("Profile uses Lua conditions, but the DevKitLuaRules plugin is disabled");
		return nullptr;
#endif
	}
	FTCHARToUTF8 Bytes(*(Json + TEXT("\n") + Lua));
	Set->Revision = FMD5::HashBytes(reinterpret_cast<const uint8*>(Bytes.Get()), Bytes.Length());
	return Set;
}

FDevKitActionRuleDecision FDevKitActionRuleSet::Evaluate(const FGameplayTagContainer& RequestTags,
	const FGameplayTagContainer& OwnedTags, const FString& Subject, const TMap<FString,double>& Values) const
{
	FDevKitActionRuleDecision D; D.ProfileId = Id; D.Revision = Revision;
	const FDevKitActionDefinition* Request = nullptr;
	for (const auto& A : Actions)
		if (RequestTags.HasAny(A.AbilityTags))
		{
			if (Request) { D.bBlocked = D.bError = true; D.Reasons.Add(TEXT("Request matches multiple action IDs")); return D; }
			Request = &A;
		}
	if (!Request) return D;
	const TArray<int32>* RelevantRules = RulesByRequest.Find(Request->Id);
	if (!RelevantRules) return D;
#if WITH_DEVKIT_LUA_RULES
	FDevKitLuaRuleContext Context;
	Context.Subject = Subject; Context.Values = Values;
	// Lua gets explicit tags plus parents so parent queries match GAS semantics.
	for (const auto& T : OwnedTags.GetGameplayTagParents()) Context.OwnedTags.Add(T.ToString());
	for (const auto& T : RequestTags.GetGameplayTagParents()) Context.RequestTags.Add(T.ToString());
#endif
	TMap<FString, bool> ConditionResults;
	TMap<FString, FString> ConditionErrors;
	for (const int32 RuleIndex : *RelevantRules)
	{
		const FDevKitActionRule& R = Rules[RuleIndex];
		if (!R.bEnabled || (R.Subject != TEXT("any") && R.Subject != Subject)) continue;
		const FDevKitActionDefinition& Current = Actions[ActionIndices.FindChecked(R.Current)];
		if (!OwnedTags.HasAny(Current.StateTags)) continue;
		bool bMatch = true; FString Error;
		if (!R.Condition.IsEmpty())
		{
			if (const bool* Cached = ConditionResults.Find(R.Condition)) bMatch = *Cached;
			else if (const FString* CachedError = ConditionErrors.Find(R.Condition)) Error = *CachedError;
			else if (ConditionResults.Num() + ConditionErrors.Num() >= 16) Error = TEXT("Request exceeds 16 distinct Lua condition evaluations");
#if WITH_DEVKIT_LUA_RULES
			else if (!VM || !VM->EvaluateCondition(R.Condition, Context, bMatch, Error))
			{
				if (Error.IsEmpty()) Error = TEXT("Lua runtime unavailable");
				ConditionErrors.Add(R.Condition, Error);
			}
			else ConditionResults.Add(R.Condition, bMatch);
#else
			else
			{
				Error = TEXT("Lua runtime unavailable");
				ConditionErrors.Add(R.Condition, Error);
			}
#endif
		}
		if (!Error.IsEmpty())
		{
			D.bBlocked = D.bError = true; D.MatchedRules.Add(R.Id);
			D.Reasons.Add(R.Id + TEXT(": Lua error: ") + Error); continue;
		}
		if (!bMatch) continue;
		D.MatchedRules.Add(R.Id); D.Reasons.Add(R.Reason);
		if (R.Effect == TEXT("block")) D.bBlocked = true;
		else D.CancelAbilityTags.AppendTags(Current.AbilityTags);
	}
	if (D.bBlocked) D.CancelAbilityTags.Reset(); // Denial never cancels the current action.
	return D;
}
