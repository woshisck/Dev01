using UnrealBuildTool;
using System.IO;

public class YogVisionRuntime : ModuleRules
{
    public YogVisionRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "DeveloperSettings", "Niagara" });
        PrivateDependencyModuleNames.AddRange(new[] { "RenderCore", "RHI", "Renderer", "Projects" });
        // Only editor builds query the active level viewport for the native
        // Ninja fog interaction centre; packaged runtime uses the local pawn.
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }
        // The exact native fog finish bridge uses a branch-specific private
        // renderer interface. Installed Builds may omit it; the independent
        // native volume and simulation paths only need the public interfaces.
        string RendererPrivateDirectory = Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Private");
        bool bHasNativeAirFogBridge = File.Exists(Path.Combine(RendererPrivateDirectory, "SceneRendering.h"));
        PrivateDefinitions.Add("YOG_WITH_NATIVE_AIR_FOG_BRIDGE=" + (bHasNativeAirFogBridge ? "1" : "0"));
        if (bHasNativeAirFogBridge)
        {
            PrivateIncludePaths.Add(RendererPrivateDirectory);
            PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/Renderer/Internal"));
        }
    }
}
