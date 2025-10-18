// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RococoGui : ModuleRules
{
	public RococoGui(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bEnableExceptions = true;

        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		this.ForceIncludeFiles.Add("rococo.UE5.h");
		
		if (Target.LinkType == TargetLinkType.Monolithic)
        {
            PublicDefinitions.Add("ROCOCO_BUILD_IS_MONOLITHIC");
        }
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "Engine",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"ApplicationCore",
				"UMG",
				"LibTiff",
				"zlib"
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
