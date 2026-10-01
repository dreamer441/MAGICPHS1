using UnrealBuildTool;
public class PhysicalBody : ModuleRules
{
    public PhysicalBody(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "MaterialCore" });
    }
}
