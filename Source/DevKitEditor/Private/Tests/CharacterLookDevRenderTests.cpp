#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "ShaderCompiler.h"
#include "Tools/CharacterLookDev/DevKitCharacterLookDevService.h"

namespace
{
const TCHAR* RenderSubject = TEXT("/Game/Art/Character/Elian/SK_Charactor_Elian.SK_Charactor_Elian");
const TCHAR* RenderSubjectPackage = TEXT("/Game/Art/Character/Elian/SK_Charactor_Elian");
constexpr double TotalTimeoutSeconds = 90.0;
constexpr double SettleSeconds = 2.0;

TSet<FString> ReportNames(const FString& Directory)
{
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Directory / TEXT("LookDev_*.json")), true, false);
	TSet<FString> Result;
	for (const FString& File : Files) Result.Add(File);
	return Result;
}

/** One nonblocking state machine, bounded across all four captures, in a disposable editor process. */
class FDevKitLookDevRenderCommand final : public IAutomationLatentCommand
{
public:
	explicit FDevKitLookDevRenderCommand(FAutomationTestBase* InTest)
		: Test(InTest), Began(FPlatformTime::Seconds())
	{
	}

	virtual bool Update() override
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - Began >= TotalTimeoutSeconds)
		{
			return Fail(FString::Printf(TEXT("LookDev RenderSmoke exceeded total 90 seconds at %s, state %d. Pending screenshot: %s"),
				*Presets[PresetIndex], static_cast<int32>(State), *ExpectedPng));
		}
		FText Message;
		switch (State)
		{
		case EState::Setup:
		{
			if (!FDevKitCharacterLookDevService::CreateStage(TEXT("Neutral"), false, Message)) return Fail(Message.ToString());
			USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, RenderSubject);
			if (!Mesh) return Fail(FString::Printf(TEXT("Cannot load actual character mesh: %s"), RenderSubject));
			if (!FDevKitCharacterLookDevService::SetSubject(Mesh, Message)) return Fail(Message.ToString());
			// Elian's imported mesh faces +Y; align it with the -X close-view camera for this fixture.
			if (!FDevKitCharacterLookDevService::RotateSubject(90.f, Message)) return Fail(Message.ToString());
			if (!FDevKitCharacterLookDevService::SetCamera(false, Message))
				return Fail(TEXT("Render test process has no usable level perspective viewport: ") + Message.ToString());
			Test->AddInfo(FString::Printf(TEXT("LookDev RenderSmoke actual subject: %s. Reference pose; original mesh materials."), RenderSubject));
			State = EState::Apply;
			return false;
		}
		case EState::Apply:
			if (!FDevKitCharacterLookDevService::ApplyPreset(Presets[PresetIndex], Message)) return Fail(Message.ToString());
			StableSince = 0;
			StableUpdates = 0;
			State = EState::Settle;
			return false;
		case EState::Settle:
			// Let ordinary editor ticks process shader results and scene captures. No Sleep or blocking FinishAllCompilation.
			if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
			{
				StableSince = 0;
				StableUpdates = 0;
				return false;
			}
			if (StableSince == 0) StableSince = Now;
			++StableUpdates;
			if (Now - StableSince < SettleSeconds || StableUpdates < 20) return false;
			return RequestCapture();
		case EState::WaitForPng:
		{
			const int64 Size = IFileManager::Get().FileSize(*ExpectedPng);
			if (Size <= 32) return false;
			// Require the completed file to stabilize across ticks before reading an asynchronous image write.
			if (Size != LastPngSize)
			{
				LastPngSize = Size;
				PngStableSince = Now;
				return false;
			}
			if (Now - PngStableSince < 0.5) return false;
			TArray<uint8> Bytes;
			if (!FFileHelper::LoadFileToArray(Bytes, *ExpectedPng)) return false;
			const uint8 Signature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
			if (Bytes.Num() < 32 || FMemory::Memcmp(Bytes.GetData(), Signature, sizeof(Signature)) != 0)
				return Fail(TEXT("Capture output is not a nonempty PNG: ") + ExpectedPng);
			const uint8* Data = Bytes.GetData();
			const uint32 Width = (uint32(Data[16]) << 24) | (uint32(Data[17]) << 16) | (uint32(Data[18]) << 8) | uint32(Data[19]);
			const uint32 Height = (uint32(Data[20]) << 24) | (uint32(Data[21]) << 16) | (uint32(Data[22]) << 8) | uint32(Data[23]);
			if (Width == 0 || Height == 0) return Fail(TEXT("Capture PNG has empty dimensions: ") + ExpectedPng);
			Test->AddInfo(FString::Printf(TEXT("LOOKDEV_CAPTURE_COMPLETE preset=%s file=%s bytes=%lld width=%u height=%u"),
				*Presets[PresetIndex], *ExpectedPng, Size, Width, Height));
			++PresetIndex;
			if (PresetIndex == Presets.Num())
			{
				Test->AddInfo(TEXT("Four actual PNG files verified. Render smoke checks capture delivery, not art approval, shader parity or gameplay lighting certification."));
				return true;
			}
			State = EState::Apply;
			return false;
		}
		}
		return Fail(TEXT("Unknown LookDev render smoke state."));
	}

private:
	bool Fail(const FString& Reason)
	{
		Test->AddError(Reason);
		return true;
	}

	bool RequestCapture()
	{
		const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("CharacterLookDev"));
		const TSet<FString> BeforeReports = ReportNames(Directory);
		TArray<FString> BeforePng;
		IFileManager::Get().FindFiles(BeforePng, *(Directory / TEXT("*.png")), true, false);
		FText Message;
		if (!FDevKitCharacterLookDevService::Capture(Message)) return Fail(Message.ToString());
		ExpectedPng.Reset();
		for (const FString& Name : ReportNames(Directory))
		{
			if (BeforeReports.Contains(Name)) continue;
			FString Text;
			TSharedPtr<FJsonObject> Json;
			if (!FFileHelper::LoadFileToString(Text, *(Directory / Name))
				|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json.IsValid()) continue;
			FString Id, Status, Path;
			if (Json->TryGetStringField(TEXT("preset_id"), Id) && Id == Presets[PresetIndex]
				&& Json->TryGetStringField(TEXT("capture_status"), Status) && Status == TEXT("requested_not_verified_complete")
				&& Json->TryGetStringField(TEXT("capture_path"), Path) && !Path.IsEmpty())
			{
				ExpectedPng = Path;
				break;
			}
		}
		if (ExpectedPng.IsEmpty()) return Fail(TEXT("Capture request did not produce a matching evidence report with its exact PNG path."));
		if (BeforePng.Contains(FPaths::GetCleanFilename(ExpectedPng))) return Fail(TEXT("Capture reused an existing PNG; cannot claim a new rendered result: ") + ExpectedPng);
		Test->AddInfo(FString::Printf(TEXT("LOOKDEV_CAPTURE_REQUESTED preset=%s file=%s"), *Presets[PresetIndex], *ExpectedPng));
		LastPngSize = -1;
		PngStableSince = 0;
		State = EState::WaitForPng;
		return false;
	}

	enum class EState { Setup, Apply, Settle, WaitForPng };
	FAutomationTestBase* Test;
	const TArray<FString> Presets{ TEXT("Neutral"), TEXT("L1_Torch"), TEXT("L2_Fire"), TEXT("L3_Sanctuary") };
	EState State = EState::Setup;
	double Began;
	double StableSince = 0;
	double PngStableSince = 0;
	int32 PresetIndex = 0;
	int32 StableUpdates = 0;
	int64 LastPngSize = -1;
	FString ExpectedPng;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLookDevRenderSmokeTest,
	"DevKitEditor.CharacterLookDev.RenderSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLookDevRenderSmokeTest::RunTest(const FString& Parameters)
{
	if (!FApp::IsUnattended() || !FParse::Param(FCommandLine::Get(), TEXT("Dev01LookDevRender")))
	{
		AddInfo(TEXT("Skipped: RenderSmoke requires -Unattended -Dev01LookDevRender in a dedicated real-RHI editor process. Current StylusInput is incompatible with -RenderOffscreen."));
		return true;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("NullRHI")))
	{
		AddError(TEXT("RenderSmoke requires a real rendering device; -NullRHI cannot validate PNG captures."));
		return false;
	}
	if (!FPackageName::DoesPackageExist(RenderSubjectPackage))
	{
		AddError(FString::Printf(TEXT("Actual character asset is missing: %s"), RenderSubjectPackage));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FDevKitLookDevRenderCommand(this));
	return true;
}

#endif
