using UnrealBuildTool;
public class dz1Editor : ModuleRules
{
    public dz1Editor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "dz1", "UMG" });
        PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "BlueprintGraph", "Kismet", "KismetCompiler", "AnimGraph", "AnimGraphRuntime", "EnhancedInput", "InputCore", "AssetRegistry" });
    }
}
