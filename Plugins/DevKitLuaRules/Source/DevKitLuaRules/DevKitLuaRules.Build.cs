using UnrealBuildTool;
using System.IO;

public class DevKitLuaRules : ModuleRules
{
    public DevKitLuaRules(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false; // Pristine Lua C translation units must remain separate.
        // Upstream Lua's protected longjmp error paths deliberately end in unreachable returns.
        // Keep the vendor sources pristine and retain this diagnostic without promoting it to an error.
        CppCompileWarningSettings.UnreachableCodeWarningLevel = WarningLevel.Warning;
        PublicDependencyModuleNames.Add("Core");
        PrivateDefinitions.Add("_CRT_SECURE_NO_WARNINGS=1");
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private", "ThirdParty", "Lua"));
        RuntimeDependencies.Add("$(PluginDir)/ThirdPartyNotices/Lua-LICENSE.txt", StagedFileType.NonUFS);
    }
}
