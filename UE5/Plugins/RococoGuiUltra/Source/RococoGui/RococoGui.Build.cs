// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RococoGui : ModuleRules
{
	public RococoGui(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bEnableExceptions = true;

        PublicIncludePaths.AddRange(
			new string[] {
				}
			);
			
		var jpegDir = new EpicGames.Core.DirectoryReference(Path.Join(PluginDirectory, "Source/ThirdParty/jpeg-6b"));
				
		ConditionalAddModuleDirectory(jpegDir);				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"ThirdParty/jpeg-6b"
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
