using UnrealBuildTool;

public class SpellPreview : ModuleRules
{
    public SpellPreview(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "SpellPattern",
            "Core", "CoreUObject", "Engine", "SpellCreation", "LiveSpellCasting", "PlayerViewModes", "InnerRealm", "SpellExecution"
        });
    }
}
