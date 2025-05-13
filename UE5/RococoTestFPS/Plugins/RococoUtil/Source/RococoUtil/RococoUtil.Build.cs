// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RococoUtil : ModuleRules
{
	public string RococoIncludeDirectory
	{
		get
		{
			string dir = PluginDirectory;
			string fullPath = dir;
			string lastFullPath;

			do
			{
                lastFullPath = fullPath;

                fullPath = Path.GetFullPath(Path.Combine(fullPath, ".."));

				string includeDirectory = Path.Combine(fullPath, "source/rococo/include/");
				if (Directory.Exists(includeDirectory))
				{
					return includeDirectory;
				}

            } while(lastFullPath != fullPath);

			throw new System.Exception("Could not find rococo directory by enumerating ancestors of " + dir);
		}
	}

	public RococoUtil(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
                RococoIncludeDirectory
            }
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
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
				// ... add private dependencies that you statically link with here ...	
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
