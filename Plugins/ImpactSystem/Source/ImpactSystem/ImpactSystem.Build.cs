using UnrealBuildTool;
public class ImpactSystem : ModuleRules
{
    public ImpactSystem(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "MaterialCore", "PhysicalBody" });
    }
}
