using UnrealBuildTool;

public class SpellExecution : ModuleRules
{
    public SpellExecution(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "SpellMotion",
            "SpellPattern",
            "SpellLoadout",
            "Core", "CoreUObject", "Engine", "InputCore",
            "SpellCreation", "LiveSpellCasting", "PlayerViewModes", "EarthMagic"
        });
    }
}
