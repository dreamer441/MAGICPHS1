using UnrealBuildTool;

public class LiveSpellCasting : ModuleRules
{
    public LiveSpellCasting(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "SpellCreation"
        });
    }
}
