using UnrealBuildTool;
public class GaussianSplatProbe : ModuleRules
{
    public GaussianSplatProbe(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "Json" });
    }
}
