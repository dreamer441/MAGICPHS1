using UnrealBuildTool;

public class SpellMotion : ModuleRules
{
    public SpellMotion(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "SpellCreation"
        });
    }
}
