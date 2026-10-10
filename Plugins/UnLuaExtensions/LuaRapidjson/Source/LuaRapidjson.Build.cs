// Tencent is pleased to support the open source community by making UnLua available.
// 
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the MIT License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

using System;
using System.IO;
using UnrealBuildTool;

public class LuaRapidjson : ModuleRules
{
    public LuaRapidjson(ReadOnlyTargetRules Target) : base(Target)
    {
#if UE_5_2_OR_LATER
        IWYUSupport = IWYUSupport.None;
#else
        bEnforceIWYU = false;
#endif
        bUseUnity = false;
        // rapidjson.h declares a global `SizeType` that shadows the one engine containers declare in
        // their own template scopes. The shared PCH is built with /we4459 and restores that state over
        // our /wd4459, so the only way to suppress it is to not pull the engine templates in at all.
        PCHUsage = PCHUsageMode.NoSharedPCHs;
#if UE_5_6_OR_LATER
        CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off;
        CppCompileWarningSettings.ShadowVariableWarningLevel = WarningLevel.Off;
#else
        bEnableUndefinedIdentifierWarnings = false;
#endif
        bEnableExceptions = true;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
            });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "UnLua",
            "Lua"
        });

        PrivateDefinitions.AddRange(
            new[]
            {
                "LUA_LIB"
            }
        );
        
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "src"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));
    }
}