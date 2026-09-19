using UnrealBuildTool;
using System.Collections.Generic;

public class LalalandUnrealTarget : TargetRules
{
    public LalalandUnrealTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("LalalandUnreal");
    }
}
