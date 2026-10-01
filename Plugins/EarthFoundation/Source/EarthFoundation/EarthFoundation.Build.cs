using UnrealBuildTool;
public class EarthFoundation : ModuleRules
{
    public EarthFoundation(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "PhysicsCore", "GeometryCore", "GeometryFramework", "MaterialCore", "ImpactSystem"
        });
    }
}
