using UnrealBuildTool;

public class LalalandUnreal : ModuleRules
{
    public LalalandUnreal(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "HTTP", "WebSockets", "Json", "JsonUtilities",
            "UMG", "Slate", "SlateCore", "AIModule", "NavigationSystem", "AssetRegistry", "PhysicsCore"
        });
        RuntimeDependencies.Add("$(ProjectDir)/Content/Characters/PLAYER_v024/grip.json", StagedFileType.NonUFS);
    }
}
