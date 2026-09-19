using UnrealBuildTool;
using System.Collections.Generic;

public class LalalandUnrealEditorTarget : TargetRules
{
    public LalalandUnrealEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("LalalandUnreal");
    }
}
