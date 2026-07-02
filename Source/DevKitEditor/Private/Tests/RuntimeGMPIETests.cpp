#if WITH_DEV_AUTOMATION_TESTS

#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "System/YogRuntimeGMSubsystem.h"
#include "Tests/AutomationEditorCommon.h"

namespace
{
constexpr double RuntimeGMPIEStartTimeoutSeconds = 30.0;

APlayerController* GetRuntimeGMPIEPlayerController()
{
	UWorld* PlayWorld = GEditor ? GEditor->PlayWorld : nullptr;
	return PlayWorld ? PlayWorld->GetFirstPlayerController() : nullptr;
}

class FWaitForRuntimeGMPIEPlayerControllerCommand final : public IAutomationLatentCommand
{
public:
	explicit FWaitForRuntimeGMPIEPlayerControllerCommand(FAutomationTestBase* InTest)
		: Test(InTest)
		, StartTime(FPlatformTime::Seconds())
	{
	}

	virtual bool Update() override
	{
		if (GetRuntimeGMPIEPlayerController())
		{
			return true;
		}

		if (FPlatformTime::Seconds() - StartTime > RuntimeGMPIEStartTimeoutSeconds)
		{
			Test->AddError(TEXT("PIE player controller was not ready before timeout."));
			return true;
		}

		return false;
	}

private:
	FAutomationTestBase* Test = nullptr;
	double StartTime = 0.0;
};

class FPressRuntimeGMPIEF12Command final : public IAutomationLatentCommand
{
public:
	explicit FPressRuntimeGMPIEF12Command(FAutomationTestBase* InTest)
		: Test(InTest)
	{
	}

	virtual bool Update() override
	{
		APlayerController* PlayerController = GetRuntimeGMPIEPlayerController();
		UGameInstance* GameInstance = PlayerController ? PlayerController->GetGameInstance() : nullptr;
		UYogRuntimeGMSubsystem* RuntimeGM = GameInstance ? GameInstance->GetSubsystem<UYogRuntimeGMSubsystem>() : nullptr;

		Test->TestNotNull(TEXT("PIE player controller"), PlayerController);
		Test->TestNotNull(TEXT("PIE Runtime GM subsystem"), RuntimeGM);
		Test->TestTrue(TEXT("Slate application is initialized"), FSlateApplication::IsInitialized());

		if (!PlayerController || !RuntimeGM || !FSlateApplication::IsInitialized())
		{
			return true;
		}

		RuntimeGM->CloseGMPanel(PlayerController);

		const FKeyEvent KeyEvent(EKeys::F12, FModifierKeysState(), 0, false, 0, 0);
		const bool bHandled = FSlateApplication::Get().ProcessKeyDownEvent(KeyEvent);

		Test->TestTrue(TEXT("PIE Slate F12 event is handled by Runtime GM input path"), bHandled);
		Test->TestTrue(TEXT("PIE Slate F12 opens Runtime GM panel"), RuntimeGM->IsGMPanelOpen());

		RuntimeGM->CloseGMPanel(PlayerController);
		return true;
	}

private:
	FAutomationTestBase* Test = nullptr;
};

class FExecuteRuntimeGMConsoleFromEditorWorldCommand final : public IAutomationLatentCommand
{
public:
	explicit FExecuteRuntimeGMConsoleFromEditorWorldCommand(FAutomationTestBase* InTest)
		: Test(InTest)
	{
	}

	virtual bool Update() override
	{
		APlayerController* PlayerController = GetRuntimeGMPIEPlayerController();
		UGameInstance* GameInstance = PlayerController ? PlayerController->GetGameInstance() : nullptr;
		UYogRuntimeGMSubsystem* RuntimeGM = GameInstance ? GameInstance->GetSubsystem<UYogRuntimeGMSubsystem>() : nullptr;
		UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;

		Test->TestNotNull(TEXT("PIE player controller"), PlayerController);
		Test->TestNotNull(TEXT("PIE Runtime GM subsystem"), RuntimeGM);
		Test->TestNotNull(TEXT("Editor world for Output Log command context"), EditorWorld);
		if (!PlayerController || !RuntimeGM || !EditorWorld)
		{
			return true;
		}

		RuntimeGM->CloseGMPanel(PlayerController);
		IConsoleManager::Get().ProcessUserConsoleInput(TEXT("Yog.GM"), *GLog, EditorWorld);

		Test->TestTrue(TEXT("Yog.GM opens Runtime GM panel when executed from editor command context"), RuntimeGM->IsGMPanelOpen());

		RuntimeGM->CloseGMPanel(PlayerController);
		return true;
	}

private:
	FAutomationTestBase* Test = nullptr;
};

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeGMPIEF12OpensPanelTest,
	"DevKitEditor.RuntimeGM.PIEF12OpensPanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRuntimeGMPIEF12OpensPanelTest::RunTest(const FString& Parameters)
{
	AddExpectedError(TEXT("Null input trigger detected in mapping to input action 'IA_CardCache'"), EAutomationExpectedErrorFlags::Contains, 1);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForRuntimeGMPIEPlayerControllerCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FPressRuntimeGMPIEF12Command(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeGMConsoleCommandRegisteredTest,
	"DevKitEditor.RuntimeGM.ConsoleCommandRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRuntimeGMConsoleCommandRegisteredTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Yog.GM console command is registered"),
		IConsoleManager::Get().FindConsoleObject(TEXT("Yog.GM")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeGMConsoleCommandFromEditorWorldOpensPanelTest,
	"DevKitEditor.RuntimeGM.ConsoleCommandFromEditorWorldOpensPanel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRuntimeGMConsoleCommandFromEditorWorldOpensPanelTest::RunTest(const FString& Parameters)
{
	AddExpectedError(TEXT("Null input trigger detected in mapping to input action 'IA_CardCache'"), EAutomationExpectedErrorFlags::Contains, 1);

	ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/Art/Map/Map_Data/L1_InitialRoom/InitialRoom")));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForRuntimeGMPIEPlayerControllerCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FExecuteRuntimeGMConsoleFromEditorWorldCommand(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

#endif
