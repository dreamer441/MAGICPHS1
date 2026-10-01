using UnrealBuildTool;

public class SpellPattern : ModuleRules
{
    public SpellPattern(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "SpellCreation"
        });
    }
}
