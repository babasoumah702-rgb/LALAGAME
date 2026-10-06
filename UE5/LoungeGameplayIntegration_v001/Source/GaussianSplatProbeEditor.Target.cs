using UnrealBuildTool;
public class GaussianSplatProbeEditorTarget : TargetRules
{
    public GaussianSplatProbeEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("GaussianSplatProbe");
    }
}
