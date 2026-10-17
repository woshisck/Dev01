#include "ActionRules/DevKitActionRuleSubsystem.h"
#include "Misc/Paths.h"

bool UDevKitActionRuleSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}
void UDevKitActionRuleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bEnabled = GetDefault<UDevKitActionRuleSettings>()->bEnabled;
	if (bEnabled) LoadConfigured(LastError);
}
void UDevKitActionRuleSubsystem::Deinitialize()
{
	Snapshot.Reset(); LastError.Reset(); bEnabled = false; Super::Deinitialize();
}
bool UDevKitActionRuleSubsystem::ResolveProfilePath(const FString& RelativePath, FString& FullPath, FString& Error)
{
	Error.Reset();
	const FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("ActionRules/"));
	if (RelativePath.IsEmpty() || !FPaths::IsRelative(RelativePath) || RelativePath.Contains(TEXT(":"))
		|| FPaths::GetExtension(RelativePath) != TEXT("json"))
	{ Error = TEXT("Choose a .json profile relative to Content/ActionRules"); return false; }
	FullPath = FPaths::ConvertRelativePathToFull(Root / RelativePath);
	FPaths::NormalizeFilename(FullPath); FPaths::CollapseRelativeDirectories(FullPath);
	if (!FPaths::IsUnderDirectory(FullPath, Root)) { Error = TEXT("Profile is outside Content/ActionRules"); return false; }
	return true;
}
bool UDevKitActionRuleSubsystem::LoadConfigured(FString& Error)
{
	FString Path;
	if (!ResolveProfilePath(GetDefault<UDevKitActionRuleSettings>()->Profile, Path, Error)) return false;
	auto Candidate = FDevKitActionRuleSet::LoadFile(Path, Error);
	if (!Candidate) return false;
	Snapshot = Candidate; LastError.Reset(); return true;
}
bool UDevKitActionRuleSubsystem::ReloadRules(FString& Error)
{
#if UE_BUILD_SHIPPING
	Error = TEXT("Runtime rule reload is disabled in Shipping"); return false;
#else
	if (!bEnabled) { Error = TEXT("Enable Action Rules before starting this world"); return false; }
	if (LoadConfigured(Error)) return true;
	LastError = Error; return false;
#endif
}
FString UDevKitActionRuleSubsystem::GetStatus() const
{
	if (!bEnabled) return TEXT("Disabled - legacy rules remain active");
	if (!Snapshot) return TEXT("Rule load failed (requests denied): ") + LastError;
	return Snapshot->Id + TEXT(" / ") + Snapshot->Revision + (LastError.IsEmpty() ? FString() : TEXT(" / Last reload rejected: ") + LastError);
}
FDevKitActionRuleDecision UDevKitActionRuleSubsystem::Evaluate(const FGameplayTagContainer& RequestTags,
	const FGameplayTagContainer& OwnedTags, const FString& Subject, const TMap<FString,double>& Values) const
{
	if (!bEnabled) return {};
	if (Snapshot) return Snapshot->Evaluate(RequestTags, OwnedTags, Subject, Values);
	FDevKitActionRuleDecision D; D.bBlocked = D.bError = true; D.Reasons.Add(LastError); return D;
}
