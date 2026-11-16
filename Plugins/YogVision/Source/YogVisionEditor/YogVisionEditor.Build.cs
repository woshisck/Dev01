using UnrealBuildTool;

public class YogVisionEditor : ModuleRules
{
    public YogVisionEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "Slate", "SlateCore", "YogVisionRuntime", "DeveloperSettings", "UnrealEd"
        });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "EditorFramework", "LevelEditor", "InputCore", "PropertyEditor",
            "Projects", "AssetRegistry", "AssetTools", "RenderCore", "RHI", "Json", "UMG", "Niagara"
        });
    }
}
