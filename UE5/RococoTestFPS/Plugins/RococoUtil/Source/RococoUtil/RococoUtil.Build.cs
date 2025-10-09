// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Configuration;
using System.Drawing;
using System.IO;
using System.Text;
using UnrealBuildTool;
public class RococoUtil : ModuleRules
{
    public RococoUtil(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bEnableExceptions = true;

        if (Target.LinkType == TargetLinkType.Monolithic)
        {
            PublicDefinitions.Add("ROCOCO_BUILD_IS_MONOLITHIC");
        }

        string rococoInclude = Environment.GetEnvironmentVariable("Rococo-Include");
        if (rococoInclude == null)
        {
            throw new Exception("Expecting Rococo-Include@Env to have been defined by RococoBuild.Build.cs");
        }

        string sexyInclude = Environment.GetEnvironmentVariable("Sexy-Include");
        if (sexyInclude == null)
        {
            throw new Exception("Expecting Sexy-Include@Env to have been defined by RococoBuild.Build.cs");
        }

        PublicIncludePaths.AddRange(
			new string[] {
                rococoInclude,
                sexyInclude
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
                "Engine"
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
				"ApplicationCore",
				"RococoOS"
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
