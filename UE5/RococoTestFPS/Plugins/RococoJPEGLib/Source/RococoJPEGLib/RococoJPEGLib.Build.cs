// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Net.NetworkInformation;
using System.Text;
using UnrealBuildTool;

public class RococoJPEGLib : ModuleRules
{
    public RococoJPEGLib(ReadOnlyTargetRules Target) : base(Target)
	{
        bEnableExceptions = true;

        if (Target.LinkType == TargetLinkType.Monolithic)
        {
            PublicDefinitions.Add("ROCOCO_BUILD_IS_MONOLITHIC");
        }

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");

        string rococoInclude = Environment.GetEnvironmentVariable("Rococo-Include");
        if (rococoInclude == null)
        {
            throw new Exception("Expecting Rococo-Include@Env to have been defined by RococoBuild.Build.cs");
        }

		// Include directory will be in $(rococo)source/rococo/include and we want $(rococo)source/3rd-Party...
		string jpeg6 = Path.Combine(rococoInclude, "..", "..", "3rd-Party", "libjpg", "jpeg-6b");

        PublicIncludePaths.AddRange(
			new string[] {
                rococoInclude,
                jpeg6
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
