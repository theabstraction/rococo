// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RococoTestFPS : ModuleRules
{
    private string rococoIncludeDirectory;
    private string rococoSexyCommonDirectory;
    private string rococoHomeDirectory;
    private string rococoSourceDirectory;

    private void PrepRococoDirectories()
    {
        if (rococoIncludeDirectory == null)
        {
            string dir = ModuleDirectory;
            string fullPath = dir;
            string lastFullPath;

            System.Console.WriteLine("ModuleDirectory: {0}", dir);

            do
            {
                lastFullPath = fullPath;

                fullPath = Path.GetFullPath(Path.Combine(fullPath, ".."));

                string candidateIncludeDirectory = Path.Combine(fullPath, "source/rococo/include/");
                if (Directory.Exists(candidateIncludeDirectory))
                {
                    rococoHomeDirectory = fullPath;
                    rococoIncludeDirectory = candidateIncludeDirectory;
                    rococoSexyCommonDirectory = Path.Combine(rococoHomeDirectory, "source/rococo/sexy/Common/");
                    rococoSourceDirectory = Path.Combine(fullPath, "source");
                    return;
                }

            } while (lastFullPath != fullPath);

            throw new System.Exception("Could not find rococo directory by enumerating ancestors of " + dir);
        }
    }
    public RococoTestFPS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" /* , "RococoOS", "RococoUtil", "RococoGui" */ });

        PrepRococoDirectories();

        System.Console.WriteLine("rococoIncludeDirectory: {0}", rococoIncludeDirectory);
        System.Console.WriteLine("rococoSexyCommonDirectory: {0}", rococoSexyCommonDirectory);

        PublicIncludePaths.AddRange(
            new string[] 
            {
                rococoIncludeDirectory,
                rococoSexyCommonDirectory
            }
        );

        PrivateIncludePaths.AddRange(
          new string[]
          {
                rococoIncludeDirectory,
                rococoSexyCommonDirectory
          }
      );
    }
}
