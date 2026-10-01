using UnrealBuildTool;

public class InnerRealm : ModuleRules
{
    public InnerRealm(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "SpellGraph",
            "WorldCodex",
            "SpellLoadout",
            "Core", "CoreUObject", "Engine", "InputCore", "Slate", "SlateCore",
            "SpellCreation", "SpellCastingBindings", "LiveSpellCasting"
        });
    }
}
