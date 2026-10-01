using UnrealBuildTool;

public class SpellGraph : ModuleRules
{
    public SpellGraph(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine",
            "SpellCreation", "WorldCodex"
        });
    }
}
