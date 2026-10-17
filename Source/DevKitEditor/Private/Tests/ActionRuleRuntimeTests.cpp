#include "Tests/ActionRuleTestAbilities.h"
#include "ActionRules/DevKitActionRuleSet.h"
#include "ActionRules/DevKitActionRuleSubsystem.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

UActionRuleTestAttack::UActionRuleTestAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	const FGameplayTagContainer Tags(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack")));
	SetAssetTags(Tags); ActivationOwnedTags = Tags;
}
UActionRuleTestDash::UActionRuleTestDash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	const FGameplayTagContainer Tags(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash")));
	SetAssetTags(Tags); ActivationOwnedTags = Tags;
}
UActionRuleTestPerExecutionDash::UActionRuleTestPerExecutionDash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

#if WITH_DEV_AUTOMATION_TESTS
struct FDevKitActionRuleTestAccess
{
	static void Install(UDevKitActionRuleSubsystem& System, TSharedPtr<const FDevKitActionRuleSet> Snapshot)
	{ System.bEnabled = true; System.Snapshot = MoveTemp(Snapshot); }
};

namespace ActionRuleTests
{
	FGameplayTagContainer Tags(const TCHAR* Name) { return FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName(Name))); }
	FString Json(const FString& Rules)
	{
		return TEXT(R"({"schemaVersion":1,"id":"isolated-tests","actions":[
{"id":"Attack","displayName":"Attack","abilityTags":["Character.State.Skill.Attack"],"stateTags":["Character.State.Skill.Attack"]},
{"id":"Dash","displayName":"Dash","abilityTags":["Character.State.Movement.Dash"],"stateTags":["Character.State.Movement.Dash"]}],"rules":[)") + Rules + TEXT("]}");
	}
	const FString Block = TEXT(R"({"id":"B","enabled":true,"subject":"any","current":"Dash","request":"Attack","effect":"block","reason":"Dash protects itself"})");
	const FString Interrupt = TEXT(R"({"id":"I","enabled":true,"subject":"any","current":"Attack","request":"Dash","effect":"interrupt","reason":"Dash cancels attack"})");
}
using namespace ActionRuleTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleDirectionTest,"DevKit.ActionRules.Core.DirectionalAndHierarchical",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FActionRuleDirectionTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto Set = FDevKitActionRuleSet::Parse(Json(Block + TEXT(",") + Interrupt), TEXT(""), Error);
	if (!TestTrue(*Error, Set.IsValid())) return false;
	const auto A = Set->Evaluate(Tags(TEXT("Character.State.Skill.Attack.Combo1")), Tags(TEXT("Character.State.Movement.Dash")), TEXT("player"), {});
	TestTrue(TEXT("Child request tag follows parent block"), A.bBlocked);
	TestTrue(TEXT("Denied request has no cancellation"), A.CancelAbilityTags.IsEmpty());
	const auto D = Set->Evaluate(Tags(TEXT("Character.State.Movement.Dash")), Tags(TEXT("Character.State.Skill.Attack.Combo1")), TEXT("player"), {});
	TestFalse(TEXT("Reverse direction admitted"), D.bBlocked);
	TestTrue(TEXT("Reverse direction plans correct cancel tags"), D.CancelAbilityTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack"))));
	TestFalse(TEXT("No current state = no restriction"), Set->Evaluate(Tags(TEXT("Character.State.Skill.Attack")), {}, TEXT("player"), {}).bBlocked);
	TestTrue(TEXT("Revision present"), !Set->Revision.IsEmpty()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleValidationTest,"DevKit.ActionRules.Core.StrictValidation",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FActionRuleValidationTest::RunTest(const FString& Parameters)
{
	FString Error;
	const TArray<FString> Invalid = {
		Json(Block + TEXT(",") + Block),
		Json(Block).Replace(TEXT("\"schemaVersion\":1"),TEXT("\"schemaVersion\":2")),
		Json(Block).Replace(TEXT("\"schemaVersion\":1"),TEXT("\"schemaVersion\":\"1\"")),
		Json(Block).Replace(TEXT("\"schemaVersion\":1"),TEXT("\"schemaVersion\":true")),
		Json(Block).Replace(TEXT("\"schemaVersion\":1"),TEXT("\"schemaVersion\":2,\"schemaVersion\":1")),
		Json(Block).Replace(TEXT("\"enabled\":true,"),TEXT("")),
		Json(Block).Replace(TEXT("\"enabled\":true"),TEXT("\"enabled\":\"false\"")),
		Json(Block).Replace(TEXT("\"enabled\":true"),TEXT("\"enabled\":1")),
		Json(Block).Replace(TEXT("\"reason\":\"Dash protects itself\""),TEXT("\"reason\":123")),
		Json(Block).Replace(TEXT("\"block\""),TEXT("\"allow\"")),
		Json(Block).Replace(TEXT("\"any\""),TEXT("\"everyone\"")),
		Json(Block).Replace(TEXT("Character.State.Skill.Attack"),TEXT("Unregistered.Action.Test")),
		Json(Block).Replace(TEXT("\"current\":\"Dash\""),TEXT("\"current\":\"Unknown\"")),
		Json(Interrupt).Replace(TEXT("\"current\":\"Attack\""),TEXT("\"current\":\"Dash\"")),
		Json(Block).Replace(TEXT("\"reason\":"),TEXT("\"typo\":")),
		Json(Block).Replace(TEXT("\"reason\":"),TEXT("\"enabled\":\"true\",\"reason\":"))
	};
	for (const FString& Candidate : Invalid)
	{ TestFalse(TEXT("Invalid profile rejected"), FDevKitActionRuleSet::Parse(Candidate, TEXT(""), Error).IsValid()); TestFalse(TEXT("Useful error"), Error.IsEmpty()); }
	FString Path;
	TestFalse(TEXT("Path traversal rejected"), UDevKitActionRuleSubsystem::ResolveProfilePath(TEXT("../bad.json"),Path,Error));
	TestFalse(TEXT("Absolute path rejected"), UDevKitActionRuleSubsystem::ResolveProfilePath(TEXT("C:/bad.json"),Path,Error));
	TestTrue(TEXT("Bundle path accepted"), UDevKitActionRuleSubsystem::ResolveProfilePath(TEXT("Examples/rules.json"),Path,Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleLuaTest,"DevKit.ActionRules.Core.LuaConditionAndFailClosed",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FActionRuleLuaTest::RunTest(const FString& Parameters)
{
	FString Error;
	const FString Conditional = Block.Replace(TEXT("\"reason\":"),TEXT("\"condition\":\"low\",\"reason\":"));
	auto Set = FDevKitActionRuleSet::Parse(Json(Conditional),TEXT("return {functions={low=function(ctx) return ctx.values.stamina < 20 end}}"), Error);
	if (!TestTrue(*Error, Set.IsValid())) return false;
	const auto Attack = Tags(TEXT("Character.State.Skill.Attack")), Dash = Tags(TEXT("Character.State.Movement.Dash"));
	TestTrue(TEXT("Low stamina matches"),Set->Evaluate(Attack,Dash,TEXT("player"),{{TEXT("stamina"),10}}).bBlocked);
	TestFalse(TEXT("Boundary does not match"),Set->Evaluate(Attack,Dash,TEXT("player"),{{TEXT("stamina"),20}}).bBlocked);
	const auto Missing = Set->Evaluate(Attack,Dash,TEXT("player"),{});
	TestTrue(TEXT("Predicate error is blocked, not allow"),Missing.bBlocked && Missing.bError);
	TestFalse(TEXT("Missing export rejected before activation"),FDevKitActionRuleSet::Parse(Json(Conditional),TEXT("return {functions={}}"),Error).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRulePrecedenceTest,"DevKit.ActionRules.Core.BlockWinsAndScope",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FActionRulePrecedenceTest::RunTest(const FString& Parameters)
{
	FString Error;
	const FString Deny = Interrupt.Replace(TEXT("\"I\""),TEXT("\"deny\"")).Replace(TEXT("\"interrupt\""),TEXT("\"block\"")).Replace(TEXT("\"any\""),TEXT("\"player\""));
	auto Set = FDevKitActionRuleSet::Parse(Json(Interrupt + TEXT(",") + Deny),TEXT(""),Error);
	if (!TestTrue(*Error,Set.IsValid())) return false;
	const auto Player = Set->Evaluate(Tags(TEXT("Character.State.Movement.Dash")),Tags(TEXT("Character.State.Skill.Attack")),TEXT("player"),{});
	TestTrue(TEXT("Any matching deny wins"),Player.bBlocked && Player.CancelAbilityTags.IsEmpty());
	const auto Enemy = Set->Evaluate(Tags(TEXT("Character.State.Movement.Dash")),Tags(TEXT("Character.State.Skill.Attack")),TEXT("enemy"),{});
	TestFalse(TEXT("Player rule does not affect enemy"),Enemy.bBlocked);
	TestFalse(TEXT("Enemy retains interrupt plan"),Enemy.CancelAbilityTags.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleGASIntegrationTest,"DevKit.ActionRules.Integration.ActivationAndCancellation",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FActionRuleGASIntegrationTest::RunTest(const FString& Parameters)
{
	const FString SelfBlock = TEXT(R"({"id":"self","enabled":true,"subject":"any","current":"Dash","request":"Dash","effect":"block","reason":"No duplicate Dash"})");
	FString Error; auto Set = FDevKitActionRuleSet::Parse(Json(Block + TEXT(",") + Interrupt + TEXT(",") + SelfBlock),TEXT(""),Error);
	if (!TestTrue(*Error,Set.IsValid())) return false;
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
	if (!TestNotNull(TEXT("Isolated world, no GameInstance/save"),World)) return false;
	auto* System = World->GetSubsystem<UDevKitActionRuleSubsystem>();
	if (!TestNotNull(TEXT("Runtime subsystem exists"),System)) { World->DestroyWorld(false); return false; }
	FDevKitActionRuleTestAccess::Install(*System,Set);
	AActor* Owner = World->SpawnActor<AActor>();
	auto* ASC = NewObject<UYogAbilitySystemComponent>(Owner); Owner->AddInstanceComponent(ASC); ASC->RegisterComponent(); ASC->InitAbilityActorInfo(Owner,Owner);
	ASC->SetConflictTable(NewObject<UStateConflictDataAsset>(ASC));
	const auto Attack = ASC->GiveAbility(FGameplayAbilitySpec(UActionRuleTestAttack::StaticClass(),1));
	const auto Dash = ASC->GiveAbility(FGameplayAbilitySpec(UActionRuleTestDash::StaticClass(),1));
	const auto Denied = ASC->GiveAbility(FGameplayAbilitySpec(UActionRuleTestDeniedDash::StaticClass(),1));
	TestTrue(TEXT("Attack starts"),ASC->TryActivateAbility(Attack));
	TestFalse(TEXT("Failed activation cannot cancel attack"),ASC->TryActivateAbility(Denied));
	TestTrue(TEXT("Attack still active after failure"),ASC->FindAbilitySpecFromHandle(Attack)->IsActive());
	UGameplayAbility* AttackInstance = ASC->FindAbilitySpecFromHandle(Attack)->GetPrimaryInstance();
	AttackInstance->SetCanBeCanceled(false);
	TestFalse(TEXT("Noncancelable target rejects incoming interrupt"),ASC->TryActivateAbility(Dash));
	TestTrue(TEXT("Protected target remains active"),ASC->FindAbilitySpecFromHandle(Attack)->IsActive());
	AttackInstance->SetCanBeCanceled(true);
	const auto Preview = ASC->QueryActionRules(Tags(TEXT("Character.State.Movement.Dash")));
	TestFalse(TEXT("Pure query plans cancellation"),Preview.CancelAbilityTags.IsEmpty());
	TestTrue(TEXT("Pure query has no effects"),ASC->FindAbilitySpecFromHandle(Attack)->IsActive());
	TestTrue(TEXT("Dash activation succeeds"),ASC->TryActivateAbility(Dash));
	TestFalse(TEXT("Attack actually canceled"),ASC->FindAbilitySpecFromHandle(Attack)->IsActive());
	TestTrue(TEXT("Self-block applies after entry, not to pre-entry interrupt snapshot"),ASC->QueryActionRules(Tags(TEXT("Character.State.Movement.Dash"))).bBlocked);
	TestFalse(TEXT("Attack blocked while dash active"),ASC->TryActivateAbility(Attack));
	ASC->CancelAbilityHandle(Dash);
	TestTrue(TEXT("No stuck block after dash ends"),ASC->TryActivateAbility(Attack));
	ASC->CancelAllAbilities();
	ASC->BlockAbilitiesWithTags(Tags(TEXT("Character.State.Movement.Dash")));
	TestFalse(TEXT("Native GAS block remains authoritative"),ASC->TryActivateAbility(Dash));
	ASC->UnBlockAbilitiesWithTags(Tags(TEXT("Character.State.Movement.Dash")));
	FDevKitActionRuleTestAccess::Install(*System,FDevKitActionRuleSet::Parse(Json(Interrupt),TEXT(""),Error));
	const auto PerExecutionDash = ASC->GiveAbility(FGameplayAbilitySpec(UActionRuleTestPerExecutionDash::StaticClass(),1));
	TestTrue(TEXT("First per-execution instance starts"),ASC->TryActivateAbility(PerExecutionDash));
	TestTrue(TEXT("Attack starts alongside test-only unblocked dash"),ASC->TryActivateAbility(Attack));
	TestTrue(TEXT("Second per-execution instance starts while spec active"),ASC->TryActivateAbility(PerExecutionDash));
	TestFalse(TEXT("Second instance captures its own interruption decision"),ASC->FindAbilitySpecFromHandle(Attack)->IsActive());
	ASC->CancelAllAbilities();
	TestTrue(TEXT("Valid Lua number input"),ASC->SetActionRuleValue(TEXT("stamina_1"),10));
	TestFalse(TEXT("Invalid input key rejected early"),ASC->SetActionRuleValue(TEXT("stamina.current"),10));
	TestFalse(TEXT("Leading digit rejected early"),ASC->SetActionRuleValue(TEXT("1stamina"),10));
	FDevKitActionRuleTestAccess::Install(*System,nullptr);
	TestTrue(TEXT("No valid snapshot fails closed"),ASC->AreAbilityTagsBlocked(Tags(TEXT("Character.State.Skill.Attack"))));
	World->DestroyWorld(false); return true;
}
#endif
