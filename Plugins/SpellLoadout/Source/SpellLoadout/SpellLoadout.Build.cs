using UnrealBuildTool;

public class SpellLoadout : ModuleRules
{
    public SpellLoadout(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "SpellCreation"
        });
    }
}
