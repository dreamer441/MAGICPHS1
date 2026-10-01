using UnrealBuildTool;

public class SpellCreation : ModuleRules
{
    public SpellCreation(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "MaterialCore", "PhysicalBody"
        });
    }
}
