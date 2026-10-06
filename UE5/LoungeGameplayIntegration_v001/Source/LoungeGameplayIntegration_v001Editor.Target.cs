using UnrealBuildTool;
public class LoungeGameplayIntegration_v001EditorTarget : TargetRules
{
    public LoungeGameplayIntegration_v001EditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "GaussianSplatProbe", "LalalandUnreal" });
    }
}
