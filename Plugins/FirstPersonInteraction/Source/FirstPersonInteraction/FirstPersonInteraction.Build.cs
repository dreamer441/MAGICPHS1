using UnrealBuildTool;

public class FirstPersonInteraction : ModuleRules
{
    public FirstPersonInteraction(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "InputCore",
            "Slate",
            "SlateCore",
            "PlayerViewModes",
            "WorldCodex",
            "EarthMagic",
            "EarthFoundation",
            "MaterialCore",
            "SpellCreation"
        });
    }
}
