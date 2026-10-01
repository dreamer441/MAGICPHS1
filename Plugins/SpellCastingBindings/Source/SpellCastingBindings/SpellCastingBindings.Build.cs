using UnrealBuildTool;

public class SpellCastingBindings : ModuleRules
{
    public SpellCastingBindings(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "SpellCreation", "LiveSpellCasting"
        });
    }
}
