using UnrealBuildTool;

public class EarthMagic : ModuleRules
{
    public EarthMagic(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "PhysicsCore",
            "MaterialCore", "PhysicalBody", "ImpactSystem", "SpellCreation",
            "EarthFoundation", "GeometryCore", "GeometryFramework"
        });
    }
}
