// Copyright 2017-2018 Backtrace I/O. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class BacktraceSettingsLibrary : ModuleRules
    {
        public BacktraceSettingsLibrary(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicIncludePaths.AddRange(
                new string[] {
                }
                );

            PrivateIncludePaths.AddRange(
                new string[] {
                }
                );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "Projects",
#if UE_4_26_OR_LATER
                    "DeveloperSettings",
#endif
                }
                );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    // "BacktraceSettingsLibrary",
                }
                );

            DynamicallyLoadedModuleNames.AddRange(
                new string[]
                {
                }
                );

            PublicIncludePathModuleNames.Add("BacktraceSettingsLibrary");
        }
    }
}
