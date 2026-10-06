using UnrealBuildTool;
public class GaussianSplatProbeTarget : TargetRules
{
    public GaussianSplatProbeTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("GaussianSplatProbe");
    }
}
