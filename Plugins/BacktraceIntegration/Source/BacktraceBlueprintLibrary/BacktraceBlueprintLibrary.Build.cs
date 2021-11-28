// Copyright 2017-2018 Backtrace I/O. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class BacktraceBlueprintLibrary : ModuleRules
    {
        public BacktraceBlueprintLibrary(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicIncludePaths.AddRange(
                new string[] {
                }
                );

            PrivateIncludePaths.AddRange(
                new string[] {
                    "BacktraceBlueprintLibrary/Private",
                }
                );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "Projects",
                    "BacktraceSettingsLibrary",
                }
                );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                }
                );

            DynamicallyLoadedModuleNames.AddRange(
                new string[]
                {
                }
                );

            PublicIncludePathModuleNames.Add("BacktraceBlueprintLibrary");
        }
    }
}
