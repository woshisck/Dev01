#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tools/ActionWorkbench/SDevKitActionRuleSourcePanel.h"
#include "Tools/ActionWorkbench/SDevKitActionWorkbenchWidget.h"
#include "ActionRules/DevKitActionRuleSubsystem.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/StyleColors.h"
#include "UObject/StrongObjectPtr.h"

struct FDevKitActionRuleSourcePanelTestAccessor
{
	static bool HasCandidate(const SDevKitActionRuleSourcePanel& Panel) { return Panel.Draft.GetCandidate().IsValid(); }
	static void Validate(SDevKitActionRuleSourcePanel& Panel, const FString& Relative)
	{ Panel.Draft.SetProfile(Relative); Panel.ValidateCandidate(); }
	static void Save(SDevKitActionRuleSourcePanel& Panel) { Panel.SaveMountSettings(); }
	static void EnableDraft(SDevKitActionRuleSourcePanel& Panel) { Panel.bDraftEnabled = true; }
	static FString Status(const SDevKitActionRuleSourcePanel& Panel) { return Panel.Status.ToString(); }
	static void Evaluate(SDevKitActionRuleSourcePanel& Panel) { Panel.EvaluateCandidate(); }
	static FString Evaluation(const SDevKitActionRuleSourcePanel& Panel) { return Panel.Evaluation.ToString(); }
	static void ChangeInputs(SDevKitActionRuleSourcePanel& Panel)
	{ Panel.SubjectInput = TEXT("enemy"); Panel.InvalidateSimulation(); }
	static void SetSubject(SDevKitActionRuleSourcePanel& Panel, const FString& Subject)
	{ Panel.SubjectInput = Subject; Panel.InvalidateSimulation(); }
	static bool IsPlaying() { return SDevKitActionRuleSourcePanel::IsPlaying(); }
};

namespace ActionRuleSourcePanelTests
{
	FString FixtureJson(bool bLua = false)
	{
		return FString(TEXT(R"({"schemaVersion":1,"id":"source-panel-fixture",)"))
			+ (bLua ? TEXT("\"luaFile\":\"conditions.lua\",") : TEXT(""))
			+ TEXT(R"("actions":[
{"id":"Attack","displayName":"普攻","abilityTags":["Character.State.Skill.Attack"],"stateTags":["Character.State.Skill.Attack"]},
{"id":"Dash","displayName":"冲刺","abilityTags":["Character.State.Movement.Dash"],"stateTags":["Character.State.Movement.Dash"]}],
"rules":[{"id":"protect_dash","enabled":true,"subject":"player","current":"Dash","request":"Attack","effect":"block","condition":")")
			+ (bLua ? TEXT("ready") : TEXT("")) + TEXT(R"(","reason":"测试冲刺保护"}]})");
	}

	struct FFixture
	{
		FString Relative, Directory, JsonPath, LuaPath;
		bool bCreated = false;
		bool Initialize(bool bLua = false)
		{
			Relative = TEXT("__source_panel_test_") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT("/rules.json");
			FString Error;
			if (!UDevKitActionRuleSubsystem::ResolveProfilePath(Relative, JsonPath, Error)) return false;
			Directory = FPaths::GetPath(JsonPath); LuaPath = Directory / TEXT("conditions.lua");
			if (IFileManager::Get().DirectoryExists(*Directory)) return false;
			bCreated = IFileManager::Get().MakeDirectory(*Directory, true);
			if (!bCreated || !FFileHelper::SaveStringToFile(FixtureJson(bLua), *JsonPath)) return false;
			return !bLua || FFileHelper::SaveStringToFile(TEXT("return { functions = { ready = function(ctx) return true end } }"), *LuaPath);
		}
		~FFixture()
		{
			if (!bCreated) return;
			// Only exact files this fixture owns; never recursively remove Content.
			IFileManager::Get().Delete(*JsonPath, false, true);
			IFileManager::Get().Delete(*LuaPath, false, true);
			IFileManager::Get().DeleteDirectory(*Directory, false, false);
		}
	};

	/** Adds only a transient context, with no world initialization/GameInstance/BeginPlay. */
	struct FScopedPlayContext
	{
		TStrongObjectPtr<UWorld> World{NewObject<UWorld>(GetTransientPackage(), NAME_None, RF_Transient)};
		FScopedPlayContext() { World->WorldType = EWorldType::PIE; GEditor->CreateNewWorldContext(EWorldType::PIE).SetCurrentWorld(World.Get()); }
		~FScopedPlayContext() { GEditor->DestroyWorldContext(World.Get()); }
	};

	class FCapture final : public IAutomationLatentCommand
	{
	public:
		FCapture(FAutomationTestBase* InTest, TSharedRef<SWidget> InWidget, FString InPrefix)
			: Test(InTest), Widget(InWidget), Prefix(MoveTemp(InPrefix)), Renderer(MakeUnique<FWidgetRenderer>(false, true)) {}
		virtual bool Update() override
		{
			const FIntPoint Sizes[] = { {1600,900}, {1024,720}, {1920,1080} };
			const float Scales[] = { 1.f, 1.f, 1.5f };
			const FIntPoint Size = Sizes[Index];
			if (!Target.IsValid())
			{
				Target.Reset(FWidgetRenderer::CreateTargetFor(FVector2D(Size), TF_Bilinear, true));
				if (!Test->TestTrue(TEXT("Native Slate capture target"), Target.IsValid())) return true;
				Started = FPlatformTime::Seconds(); Frames = 0;
			}
			Widget->Invalidate(EInvalidateWidgetReason::Layout);
			Renderer->DrawWidget(Target.Get(), Widget, Scales[Index], FVector2D(Size), 0.f);
			if (++Frames < 5 || FPlatformTime::Seconds() - Started < .4) return false;
			TArray<FColor> Pixels;
			FReadSurfaceDataFlags Flags(RCM_UNorm); Flags.SetLinearToGamma(false);
			if (!Test->TestTrue(TEXT("Read native Slate pixels"), Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels, Flags))) return true;
			const FColor Expected = FStyleColors::Panel.GetSpecifiedColor().ToFColor(true);
			const FColor Actual = Pixels[10 * Size.X + 10];
			Test->TestTrue(TEXT("UE theme colors preserve one display conversion"),
				FMath::Abs(int32(Actual.R)-Expected.R) <= 3 && FMath::Abs(int32(Actual.G)-Expected.G) <= 3 && FMath::Abs(int32(Actual.B)-Expected.B) <= 3);
			TArray64<uint8> Png;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Png);
			const FString Directory = FPaths::ProjectSavedDir() / TEXT("ActionRuleSourcesValidation/SlateCaptures");
			IFileManager::Get().MakeDirectory(*Directory, true);
			const FString File = Directory / FString::Printf(TEXT("%s_%dx%d_%d.png"), *Prefix, Size.X, Size.Y, FMath::RoundToInt(Scales[Index]*100));
			Test->TestTrue(TEXT("Save native Slate render"), FFileHelper::SaveArrayToFile(Png, *File));
			Test->AddInfo(TEXT("Native Slate render, no gameplay started: ") + File);
			Target.Reset();
			return ++Index == UE_ARRAY_COUNT(Sizes);
		}
	private:
		FAutomationTestBase* Test;
		TSharedRef<SWidget> Widget;
		FString Prefix;
		TUniquePtr<FWidgetRenderer> Renderer;
		TStrongObjectPtr<UTextureRenderTarget2D> Target;
		int32 Index = 0, Frames = 0;
		double Started = 0;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleSourceDraftValidationTest,
	"DevKit.ActionRules.UI.CandidateValidationAndPathInvalidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FActionRuleSourceDraftValidationTest::RunTest(const FString& Parameters)
{
	ActionRuleSourcePanelTests::FFixture Fixture;
	if (!TestTrue(TEXT("Create isolated JSON fixture"), Fixture.Initialize())) return false;
	FDevKitActionRuleSourceDraft Draft;
	FString Error;
	Draft.SetProfile(Fixture.Relative);
	TestFalse(TEXT("Cannot apply before validation"), Draft.RevalidateForApply(Error));
	if (!TestTrue(TEXT("Candidate validates"), Draft.Validate(Error))) { AddError(Error); return false; }
	TestTrue(TEXT("Unchanged candidate revalidates"), Draft.RevalidateForApply(Error));
	Draft.SetProfile(TEXT("../../Config/DefaultGame.ini"));
	TestFalse(TEXT("Changing path invalidates candidate"), Draft.GetCandidate().IsValid());
	TestFalse(TEXT("Out-of-root candidate rejected"), Draft.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleSourceStaleJsonTest,
	"DevKit.ActionRules.UI.RejectsChangedJsonBeforeMount", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FActionRuleSourceStaleJsonTest::RunTest(const FString& Parameters)
{
	ActionRuleSourcePanelTests::FFixture Fixture;
	if (!TestTrue(TEXT("Create isolated JSON fixture"), Fixture.Initialize())) return false;
	FDevKitActionRuleSourceDraft Draft; FString Error;
	Draft.SetProfile(Fixture.Relative);
	if (!TestTrue(TEXT("Validate initial snapshot"), Draft.Validate(Error))) return false;
	TestTrue(TEXT("Simulate external JSON edit"), FFileHelper::SaveStringToFile(ActionRuleSourcePanelTests::FixtureJson() + TEXT("\n "), *Fixture.JsonPath));
	TestFalse(TEXT("Stale candidate cannot mount"), Draft.RevalidateForApply(Error));
	TestFalse(TEXT("Stale validation cleared"), Draft.GetCandidate().IsValid());
	TestTrue(TEXT("Explicit revalidation accepts new candidate"), Draft.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleSourceStaleLuaTest,
	"DevKit.ActionRules.UI.RejectsChangedLuaBeforeMount", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FActionRuleSourceStaleLuaTest::RunTest(const FString& Parameters)
{
	ActionRuleSourcePanelTests::FFixture Fixture;
	if (!TestTrue(TEXT("Create isolated Lua/JSON fixture"), Fixture.Initialize(true))) return false;
	FDevKitActionRuleSourceDraft Draft; FString Error;
	Draft.SetProfile(Fixture.Relative);
	if (!TestTrue(TEXT("Validate Lua candidate"), Draft.Validate(Error))) { AddError(Error); return false; }
	TestTrue(TEXT("Simulate external Lua edit"), FFileHelper::SaveStringToFile(TEXT("return { functions = { ready = function(ctx) return false end } }"), *Fixture.LuaPath));
	TestFalse(TEXT("Lua drift rejects mount"), Draft.RevalidateForApply(Error));
	TestFalse(TEXT("Drift clears old snapshot"), Draft.GetCandidate().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleSourceWidgetReadOnlyTest,
	"DevKit.ActionRules.UI.WidgetReadOnlyAndSimulation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FActionRuleSourceWidgetReadOnlyTest::RunTest(const FString& Parameters)
{
	if (!TestTrue(TEXT("Slate initialized"), FSlateApplication::IsInitialized())) return false;
	const auto* Settings = GetDefault<UDevKitActionRuleSettings>();
	const bool Enabled = Settings->bEnabled;
	const FString Profile = Settings->Profile;
	FString ConfigBefore, ConfigAfter;
	const FString ConfigFile = Settings->GetDefaultConfigFilename();
	if (!TestTrue(TEXT("Read existing project config baseline"), FFileHelper::LoadFileToString(ConfigBefore, *ConfigFile))) return false;
	ActionRuleSourcePanelTests::FFixture Fixture;
	if (!TestTrue(TEXT("Create isolated simulation fixture"), Fixture.Initialize())) return false;
	TSharedRef<SDevKitActionRuleSourcePanel> Panel = SNew(SDevKitActionRuleSourcePanel);
	Panel->SlatePrepass(1.f);
	TestFalse(TEXT("Opening does not execute or validate rule code"), FDevKitActionRuleSourcePanelTestAccessor::HasCandidate(*Panel));
	FDevKitActionRuleSourcePanelTestAccessor::Validate(*Panel, Fixture.Relative);
	TestTrue(TEXT("Explicit validation loads candidate"), FDevKitActionRuleSourcePanelTestAccessor::HasCandidate(*Panel));
	FDevKitActionRuleSourcePanelTestAccessor::Evaluate(*Panel);
	TestTrue(TEXT("Read-only simulation explains matched rule"), FDevKitActionRuleSourcePanelTestAccessor::Evaluation(*Panel).Contains(TEXT("protect_dash")));
	FDevKitActionRuleSourcePanelTestAccessor::ChangeInputs(*Panel);
	TestFalse(TEXT("Editing simulation input removes stale result"), FDevKitActionRuleSourcePanelTestAccessor::Evaluation(*Panel).Contains(TEXT("protect_dash")));
	FDevKitActionRuleSourcePanelTestAccessor::SetSubject(*Panel, TEXT("any"));
	FDevKitActionRuleSourcePanelTestAccessor::Evaluate(*Panel);
	TestTrue(TEXT("Rule subject wildcard is not a runtime identity"), FDevKitActionRuleSourcePanelTestAccessor::Evaluation(*Panel).Contains(TEXT("不能使用表格通配范围 any")));
	FDevKitActionRuleSourcePanelTestAccessor::SetSubject(*Panel, TEXT("other"));
	FDevKitActionRuleSourcePanelTestAccessor::Evaluate(*Panel);
	TestTrue(TEXT("Non-player/non-enemy runtime identity is supported"), FDevKitActionRuleSourcePanelTestAccessor::Evaluation(*Panel).Contains(TEXT("候选结果：")));
	TestEqual(TEXT("Opening/validation/simulation do not enable settings"), Settings->bEnabled, Enabled);
	TestEqual(TEXT("Opening/validation/simulation do not mount profile"), Settings->Profile, Profile);
	TestTrue(TEXT("Reread config"), FFileHelper::LoadFileToString(ConfigAfter, *ConfigFile));
	TestEqual(TEXT("No config disk mutation"), ConfigAfter, ConfigBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleSourcePlayGuardTest,
	"DevKit.ActionRules.UI.PIEContextRefusesConfigWrite", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FActionRuleSourcePlayGuardTest::RunTest(const FString& Parameters)
{
	if (!GEditor || FDevKitActionRuleSourcePanelTestAccessor::IsPlaying()) { AddError(TEXT("Requires an idle editor; no real game is launched.")); return false; }
	const auto* Settings = GetDefault<UDevKitActionRuleSettings>();
	const FString Profile = Settings->Profile; const bool Enabled = Settings->bEnabled;
	FString Before, After;
	if (!TestTrue(TEXT("Config baseline"), FFileHelper::LoadFileToString(Before, *Settings->GetDefaultConfigFilename()))) return false;
	const int32 ContextsBefore = GEditor->GetWorldContexts().Num();
	TSharedRef<SDevKitActionRuleSourcePanel> Panel = SNew(SDevKitActionRuleSourcePanel);
	FDevKitActionRuleSourcePanelTestAccessor::EnableDraft(*Panel);
	{
		ActionRuleSourcePanelTests::FScopedPlayContext Play;
		TestNull(TEXT("No game instance or save subsystem"), Play.World->GetGameInstance());
		TestFalse(TEXT("No gameplay started"), Play.World->HasBegunPlay());
		FDevKitActionRuleSourcePanelTestAccessor::Save(*Panel);
		TestTrue(TEXT("Handler enforces play guard, not just disabled button"), FDevKitActionRuleSourcePanelTestAccessor::Status(*Panel).Contains(TEXT("PIE / SIE")));
	}
	TestEqual(TEXT("Transient context removed"), GEditor->GetWorldContexts().Num(), ContextsBefore);
	TestEqual(TEXT("Enabled config unchanged"), Settings->bEnabled, Enabled);
	TestEqual(TEXT("Profile config unchanged"), Settings->Profile, Profile);
	TestTrue(TEXT("Read config after rejected save"), FFileHelper::LoadFileToString(After, *Settings->GetDefaultConfigFilename()));
	TestEqual(TEXT("Disk config unchanged"), After, Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActionRuleSourceCaptureTest,
	"DevKit.ActionRules.UI.NativeSlateCapture", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FActionRuleSourceCaptureTest::RunTest(const FString& Parameters)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("ActionRuleSourcesCapture")))
	{ AddInfo(TEXT("Opt in with -ActionRuleSourcesCapture and a real RHI to generate Slate images.")); return true; }
	if (!TestTrue(TEXT("Slate initialized"), FSlateApplication::IsInitialized()) || !TestFalse(TEXT("Capture requires real RHI"), GUsingNullRHI)) return false;
	TSharedRef<SDevKitActionRuleSourcePanel> Panel = SNew(SDevKitActionRuleSourcePanel);
	FDevKitActionRuleSourcePanelTestAccessor::Validate(*Panel, TEXT("Examples/player_combat.example.json"));
	if (!TestTrue(TEXT("Bundled disabled example validates for visual inspection"), FDevKitActionRuleSourcePanelTestAccessor::HasCandidate(*Panel))) return false;
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<ActionRuleSourcePanelTests::FCapture>(this, Panel, TEXT("SourcePanel")));
	// Also capture the exact default workbench, including header/navigation. Opening
	// this follows the real menu path, but does not auto-validate or enable a profile.
	TSharedRef<SDevKitActionWorkbenchWidget> Workbench = SNew(SDevKitActionWorkbenchWidget);
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<ActionRuleSourcePanelTests::FCapture>(this, Workbench, TEXT("WorkbenchDefault")));
	return true;
}

#endif
