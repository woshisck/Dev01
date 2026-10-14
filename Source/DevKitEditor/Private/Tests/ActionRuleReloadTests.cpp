#include "ActionRules/DevKitActionRuleSet.h"
#include "ActionRules/DevKitActionRuleSubsystem.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ActionRuleReloadTests
{
    FGameplayTagContainer Attack()
    {
        return FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack")));
    }
    FGameplayTagContainer Dash()
    {
        return FGameplayTagContainer(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash")));
    }
    FString Profile(const FString& Rules)
    {
        return TEXT(R"json({"schemaVersion":1,"id":"isolated-reload","luaFile":"conditions.lua","actions":[
{"id":"Attack","displayName":"Attack","abilityTags":["Character.State.Skill.Attack"],"stateTags":["Character.State.Skill.Attack"]},
{"id":"Dash","displayName":"Dash","abilityTags":["Character.State.Movement.Dash"],"stateTags":["Character.State.Movement.Dash"]}],"rules":[)json")
            + Rules + TEXT("]}");
    }
    FString Rule(int32 Index, const FString& Condition)
    {
        return FString::Printf(TEXT(R"json({"id":"R%d","enabled":true,"subject":"any","current":"Dash","request":"Attack","effect":"block","condition":"%s","reason":"Isolated test rule"})json"), Index, *Condition);
    }

    struct FScopedBundle
    {
        FString Directory;
        FString RelativeProfile;
        FString JsonPath;
        FString LuaPath;
        bool bOwnsDirectory = false;

        bool Create()
        {
            const FString Leaf = TEXT("_AutomationReload_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
            RelativeProfile = Leaf / TEXT("rules.json");
            FString Error;
            if (!UDevKitActionRuleSubsystem::ResolveProfilePath(RelativeProfile, JsonPath, Error)) return false;
            Directory = FPaths::GetPath(JsonPath);
            LuaPath = Directory / TEXT("conditions.lua");
            if (IFileManager::Get().DirectoryExists(*Directory)) return false;
            bOwnsDirectory = IFileManager::Get().MakeDirectory(*Directory, true);
            return bOwnsDirectory;
        }
        bool WriteJson(const FString& Source) const
        {
            return bOwnsDirectory && FFileHelper::SaveStringToFile(Source, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        }
        bool WriteLua(const FString& Source) const
        {
            return bOwnsDirectory && FFileHelper::SaveStringToFile(Source, *LuaPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        }
        ~FScopedBundle()
        {
            if (bOwnsDirectory)
            {
                // Only these two files in this run's new GUID directory are owned by this test.
                // Never recursively delete Content/ActionRules, shared fixtures or unknown files.
                IFileManager::Get().Delete(*JsonPath, false, false, true);
                IFileManager::Get().Delete(*LuaPath, false, false, true);
                IFileManager::Get().DeleteDirectory(*Directory, false, false);
            }
        }
    };

    struct FScopedWorld
    {
        UWorld* World = nullptr;
        FScopedWorld()
        {
            const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
        }
        void Destroy()
        {
            if (World) { World->DestroyWorld(false); World = nullptr; }
        }
        ~FScopedWorld() { Destroy(); }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleReloadIsolationTest,
    "DevKit.ActionRules.Integration.ReloadRollbackAndWorldIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActionRuleReloadIsolationTest::RunTest(const FString& Parameters)
{
    using namespace ActionRuleReloadTests;
    FScopedBundle Bundle;
    if (!TestTrue(TEXT("Create uniquely owned rule bundle"), Bundle.Create())) return false;
    const FString Json = Profile(Rule(0, TEXT("matches")));
    const FString LuaTrue = TEXT("return {functions={matches=function(ctx) return true end}}");
    const FString LuaFalse = TEXT("return {functions={matches=function(ctx) return false end}}");
    if (!TestTrue(TEXT("Write isolated profile"), Bundle.WriteJson(Json))
        || !TestTrue(TEXT("Write isolated Lua"), Bundle.WriteLua(LuaTrue))) return false;

    UDevKitActionRuleSettings* Settings = GetMutableDefault<UDevKitActionRuleSettings>();
    const bool OriginalEnabled = Settings->bEnabled;
    const FString OriginalProfile = Settings->Profile;
    {
        // CDO values are restored on every return; never SaveConfig, load a map, create a GameInstance or start PIE.
        TGuardValue<bool> EnabledGuard(Settings->bEnabled, true);
        TGuardValue<FString> ProfileGuard(Settings->Profile, Bundle.RelativeProfile);
        FScopedWorld First;
        FScopedWorld Second;
        if (!TestNotNull(TEXT("First isolated world"), First.World) || !TestNotNull(TEXT("Second isolated world"), Second.World)) return false;
        TestNull(TEXT("First world has no GameInstance/save owner"), First.World->GetGameInstance());
        TestNull(TEXT("Second world has no GameInstance/save owner"), Second.World->GetGameInstance());
        auto* FirstRules = First.World->GetSubsystem<UDevKitActionRuleSubsystem>();
        auto* SecondRules = Second.World->GetSubsystem<UDevKitActionRuleSubsystem>();
        if (!TestNotNull(TEXT("First real subsystem"), FirstRules) || !TestNotNull(TEXT("Second real subsystem"), SecondRules)) return false;
        if (!TestTrue(TEXT("Enabled profile mounts during real Initialize"), FirstRules->GetSnapshot().IsValid())
            || !TestTrue(TEXT("Second world independently mounts"), SecondRules->GetSnapshot().IsValid())) return false;
        const FString InitialRevision = FirstRules->GetSnapshot()->Revision;
        TestTrue(TEXT("Independent per-world snapshot identities"), FirstRules->GetSnapshot().Get() != SecondRules->GetSnapshot().Get());
        TestTrue(TEXT("First initial Lua result"), FirstRules->Evaluate(Attack(), Dash(), TEXT("player"), {}).bBlocked);
        TestTrue(TEXT("Second initial Lua result"), SecondRules->Evaluate(Attack(), Dash(), TEXT("player"), {}).bBlocked);

        FString Error;
        if (!TestTrue(TEXT("Change only owned Lua candidate"), Bundle.WriteLua(LuaFalse))) return false;
        if (!TestTrue(TEXT("Real valid reload succeeds"), FirstRules->ReloadRules(Error))) return false;
        TestTrue(TEXT("Successful reload clears returned error"), Error.IsEmpty());
        TestNotEqual(TEXT("Lua change changes snapshot revision"), FirstRules->GetSnapshot()->Revision, InitialRevision);
        TestFalse(TEXT("First world executes replacement"), FirstRules->Evaluate(Attack(), Dash(), TEXT("player"), {}).bBlocked);
        TestEqual(TEXT("Second world revision remains old"), SecondRules->GetSnapshot()->Revision, InitialRevision);
        TestTrue(TEXT("Second world execution remains old"), SecondRules->Evaluate(Attack(), Dash(), TEXT("player"), {}).bBlocked);

        const TSharedPtr<const FDevKitActionRuleSet> LastGood = FirstRules->GetSnapshot();
        if (!TestTrue(TEXT("Write invalid Lua candidate"), Bundle.WriteLua(TEXT("return { broken")))) return false;
        TestFalse(TEXT("Invalid Lua reload rejected"), FirstRules->ReloadRules(Error));
        TestFalse(TEXT("Lua rejection provides reason"), Error.IsEmpty());
        TestTrue(TEXT("Lua rejection keeps exact last-known-good snapshot"), FirstRules->GetSnapshot() == LastGood);
        TestTrue(TEXT("Status exposes rejected reload"), FirstRules->GetStatus().Contains(TEXT("Last reload rejected")));
        TestTrue(TEXT("Status includes rejection reason"), FirstRules->GetStatus().Contains(Error));
        TestFalse(TEXT("Last valid Lua still executes after rejection"), FirstRules->Evaluate(Attack(), Dash(), TEXT("player"), {}).bBlocked);
        if (!TestTrue(TEXT("Write invalid JSON candidate"), Bundle.WriteJson(TEXT("{ broken")))) return false;
        TestFalse(TEXT("Invalid JSON reload rejected"), FirstRules->ReloadRules(Error));
        TestTrue(TEXT("JSON rejection preserves last-known-good snapshot"), FirstRules->GetSnapshot() == LastGood);
        TestFalse(TEXT("JSON rejection provides reason"), Error.IsEmpty());

        if (!TestTrue(TEXT("Restore valid owned JSON"), Bundle.WriteJson(Json))
            || !TestTrue(TEXT("Restore valid owned Lua"), Bundle.WriteLua(LuaTrue))) return false;
        TestTrue(TEXT("Reload recovers after invalid candidates"), FirstRules->ReloadRules(Error));
        TestFalse(TEXT("Recovery clears previous rejection status"), FirstRules->GetStatus().Contains(TEXT("Last reload rejected")));
        TestTrue(TEXT("Recovered profile executes"), FirstRules->Evaluate(Attack(), Dash(), TEXT("player"), {}).bBlocked);

        TStrongObjectPtr<UDevKitActionRuleSubsystem> RetainedFirst(FirstRules);
        First.Destroy(); // Real UWorld teardown calls subsystem Deinitialize.
        TestFalse(TEXT("Deinitialize drops the loaded snapshot"), RetainedFirst->GetSnapshot().IsValid());
        TestTrue(TEXT("Deinitialize clears enabled state and diagnostics"), RetainedFirst->GetStatus().StartsWith(TEXT("Disabled")));
        TestFalse(TEXT("Deinitialized subsystem cannot keep blocking"), RetainedFirst->Evaluate(Attack(), Dash(), TEXT("player"), {}).bBlocked);
        TestTrue(TEXT("Destroying first world leaves second alive"), SecondRules->Evaluate(Attack(), Dash(), TEXT("player"), {}).bBlocked);
    }
    TestEqual(TEXT("Enabled setting restored without SaveConfig"), Settings->bEnabled, OriginalEnabled);
    TestEqual(TEXT("Profile setting restored without SaveConfig"), Settings->Profile, OriginalProfile);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleLuaRequestBudgetTest,
    "DevKit.ActionRules.Core.LuaPerRequestBudgetAndSnapshotIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FActionRuleLuaRequestBudgetTest::RunTest(const FString& Parameters)
{
    using namespace ActionRuleReloadTests;
    FString Lua = TEXT("return {functions={");
    TArray<FString> Rows;
    for (int32 Index = 0; Index < 17; ++Index)
    {
        if (Index) Lua += TEXT(",");
        Lua += FString::Printf(TEXT("c%d=function(ctx) return false end"), Index);
        Rows.Add(Rule(Index, FString::Printf(TEXT("c%d"), Index)));
    }
    Lua += TEXT("}}");
    FString Error;
    auto OverBudget = FDevKitActionRuleSet::Parse(Profile(FString::Join(Rows, TEXT(","))), Lua, Error);
    if (!TestTrue(*Error, OverBudget.IsValid())) return false;
    const auto Denied = OverBudget->Evaluate(Attack(), Dash(), TEXT("player"), {});
    TestTrue(TEXT("17 distinct evaluated conditions fail closed"), Denied.bBlocked && Denied.bError);
    TestTrue(TEXT("Over-budget request cannot cancel anything"), Denied.CancelAbilityTags.IsEmpty());
    TestTrue(TEXT("Budget error explains limit"), FString::Join(Denied.Reasons, TEXT(" ")).Contains(TEXT("16 distinct")));
    Rows.Pop();
    auto AtBudget = FDevKitActionRuleSet::Parse(Profile(FString::Join(Rows, TEXT(","))), Lua, Error);
    if (!TestTrue(*Error, AtBudget.IsValid())) return false;
    const auto Accepted = AtBudget->Evaluate(Attack(), Dash(), TEXT("player"), {});
    TestFalse(TEXT("Exactly 16 conditions remain valid"), Accepted.bError || Accepted.bBlocked);

    Rows.Reset();
    for (int32 Index = 0; Index < 40; ++Index) Rows.Add(Rule(Index, TEXT("shared")));
    const FString SnapshotLua = TEXT(R"lua(return {functions={shared=function(ctx)
local matches = ctx.values.stamina == 10 and ctx.owned_tags["Character.State.Movement.Dash"] == true
ctx.values.stamina = 999
ctx.owned_tags["Character.State.Movement.Dash"] = nil
return matches
end}})lua");
    auto Repeated = FDevKitActionRuleSet::Parse(Profile(FString::Join(Rows, TEXT(","))), SnapshotLua, Error);
    if (!TestTrue(*Error, Repeated.IsValid())) return false;
    const FGameplayTagContainer Owned = Dash();
    const TMap<FString, double> Values = { { TEXT("stamina"), 10.0 } };
    const auto First = Repeated->Evaluate(Attack(), Owned, TEXT("player"), Values);
    TestFalse(TEXT("40 rows sharing one predicate do not exceed distinct-call budget"), First.bError);
    TestEqual(TEXT("Every row reuses matching predicate result"), First.MatchedRules.Num(), 40);
    TestEqual(TEXT("Lua cannot mutate caller values"), Values.FindChecked(TEXT("stamina")), 10.0);
    TestTrue(TEXT("Lua cannot mutate caller owned tags"), Owned.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash"))));
    const auto Second = Repeated->Evaluate(Attack(), Owned, TEXT("player"), Values);
    TestFalse(TEXT("Next request starts with clean Lua state and budget"), Second.bError);
    TestEqual(TEXT("Next request receives original snapshot"), Second.MatchedRules.Num(), 40);
    return true;
}

#endif
