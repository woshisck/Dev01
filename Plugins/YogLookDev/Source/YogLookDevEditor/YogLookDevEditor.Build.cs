using UnrealBuildTool;
using System.IO;

public class YogLookDevEditor : ModuleRules
{
    public YogLookDevEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "Slate", "SlateCore"
        });
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "UnrealEd", "EditorFramework", "LevelEditor", "EditorStyle", "InputCore",
            "PropertyEditor", "AssetRegistry", "AssetTools", "Projects", "ToolMenus",
            "RenderCore", "RHI", "Renderer", "Json", "JsonUtilities", "ApplicationCore",
            "Kismet", "BlueprintGraph", "SourceControl"
        });
    }
}
