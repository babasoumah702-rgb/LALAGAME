using UnrealBuildTool;
public class LoungeGameplayIntegration_v001Target : TargetRules
{
    public LoungeGameplayIntegration_v001Target(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new[] { "GaussianSplatProbe", "LalalandUnreal" });
    }
}
