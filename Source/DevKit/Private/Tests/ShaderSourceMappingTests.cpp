#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "ProjectDescriptor.h"
#include "ShaderCore.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectShaderSourceMappingConfigTest,
	"DevKit.UI.HUD.LiquidHealthBarShaderMappingConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectShaderSourceMappingConfigTest::RunTest(const FString& Parameters)
{
	const FString ProjectFile = FPaths::GetProjectFilePath();
	FProjectDescriptor ProjectDescriptor;
	FText FailReason;

	TestTrue(TEXT("Project descriptor loads"), ProjectDescriptor.Load(ProjectFile, FailReason));
	if (!FailReason.IsEmpty())
	{
		AddError(FString::Printf(TEXT("Project descriptor load failure: %s"), *FailReason.ToString()));
	}

	const FModuleDescriptor* ShaderModule = ProjectDescriptor.Modules.FindByPredicate([](const FModuleDescriptor& Module)
	{
		return Module.Name == TEXT("DevKitShaders");
	});

	TestNotNull(TEXT("DevKitShaders module is declared in the project descriptor"), ShaderModule);
	if (ShaderModule)
	{
		TestEqual(TEXT("DevKitShaders loads early enough to register /Project shader mappings before UI materials validate includes"),
			ShaderModule->LoadingPhase,
			ELoadingPhase::PostConfigInit);
	}

	const FString LiquidHealthShader = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders/LiquidHealthBar.ush"));
	TestTrue(TEXT("Liquid health bar shader include exists under the mapped project shader directory"),
		FPaths::FileExists(LiquidHealthShader));

	const FString ExpectedShaderDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
	const FString* ProjectShaderDir = AllShaderSourceDirectoryMappings().Find(TEXT("/Project"));
	TestNotNull(TEXT("/Project shader source mapping is registered"), ProjectShaderDir);
	if (ProjectShaderDir)
	{
		TestEqual(TEXT("/Project shader source mapping points at the project Shaders directory"),
			FPaths::ConvertRelativePathToFull(*ProjectShaderDir),
			FPaths::ConvertRelativePathToFull(ExpectedShaderDir));
	}

	return true;
}

#endif
