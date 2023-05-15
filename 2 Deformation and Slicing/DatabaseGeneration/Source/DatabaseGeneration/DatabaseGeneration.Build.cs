// Code by Marvin Kinz, m.kinz@stud.uni-heidelberg.de

using UnrealBuildTool;

public class DatabaseGeneration : ModuleRules
{
	public DatabaseGeneration(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        //Anything used in both public headers and private code should go into the PublicDependencyModuleNames array. So add additional API's here.
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Flex", "FlexLibrary", "Apex", "PhysX", "LevelEditor", "UnrealEd", "ProceduralMeshComponent", "RawMesh", "RenderCore" }); 

        PrivateDependencyModuleNames.AddRange(new string[] { "Flex", "FlexLibrary", "Apex", "PhysX", "ProceduralMeshComponent", "RawMesh" });

        PrivateIncludePathModuleNames.AddRange(new string[] { "Flex", "FlexLibrary", "Apex", "PhysX", "ProceduralMeshComponent", "RawMesh" });

        PublicIncludePaths.AddRange(
                new string[] {
                    "Flex/Public",
                    "RawMesh",
                    "AssetTools",
                    "ProceduralMeshComponent"
					// ... add public include paths required here ...
				}
                );
        //PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

        //PrivateDependencyModuleNames.AddRange(new string[] {  });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
