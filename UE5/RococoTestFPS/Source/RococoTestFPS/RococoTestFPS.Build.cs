// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class RococoTestFPS : ModuleRules
{
    private string rococoIncludeDirectory;
    private string rococoHomeDirectory;

    private static string RococoConfigPath
    {
        get
        {
            string userLocalApps = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            string rococoConfig = Path.Combine(userLocalApps, "19th-Century-Software", "Rococo.cfg");
            return rococoConfig;
        }
    }

    private static string ReadRococoHomeFromConfig()
    {
        if (File.Exists(RococoConfigPath))
        {
            string rococoPath = File.ReadAllText(RococoConfigPath);
            rococoPath = rococoPath.Trim();
            if (!Directory.Exists(rococoPath))
            {
                throw new Exception(RococoConfigPath + " exists, but the directory name within did not match a directory: " + rococoPath);
            }

            return rococoPath;
        }

        return null;
    }

    private void InitPathFromRococoHome(string _rococoHomeDirectory)
    {
        rococoHomeDirectory = _rococoHomeDirectory;
        rococoIncludeDirectory = Path.Combine(rococoHomeDirectory, "source/rococo/include/").Replace('/', Path.DirectorySeparatorChar);
    }

    private string PrivateSourceRelPath
    {
        get
        {
            return "Source/RococoGui/Private".Replace('/', Path.DirectorySeparatorChar);
        }
    }

    private void PrepRococoDirectories()
    {
        if (rococoIncludeDirectory == null)
        {
            string dir = PrivateSourceRelPath;
            string fullPath = dir;
            string lastFullPath;

            string rococoHomeFromConfig = ReadRococoHomeFromConfig();

            if (rococoHomeFromConfig != null)
            {
                InitPathFromRococoHome(rococoHomeFromConfig);
                return;
            }

            do
            {
                lastFullPath = fullPath;

                fullPath = Path.GetFullPath(Path.Combine(fullPath, ".."));

                string candidateIncludeDirectory = Path.Combine(fullPath, "source/rococo/include/").Replace('/', Path.DirectorySeparatorChar);
                if (Directory.Exists(candidateIncludeDirectory))
                {
                    InitPathFromRococoHome(fullPath);
                    return;
                }

            } while (lastFullPath != fullPath);

            throw new System.Exception("Could not find rococo directory from either " + RococoConfigPath + " or  enumerating ancestors of " + dir);
        }
    }
    public string RococoIncludeDirectory
    {
        get { return rococoIncludeDirectory; }
    }

    public string RococoHomeDirectory
    {
        get { return rococoHomeDirectory; }
    }

    public RococoTestFPS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "RococoOS", "RococoGui" });

        PrepRococoDirectories();

        System.Console.WriteLine("rococoIncludeDirectory: {0}", rococoIncludeDirectory);

        if (Target.LinkType == TargetLinkType.Monolithic)
        {
            PublicDefinitions.Add("ROCOCO_BUILD_IS_MONOLITHIC");
        }

        PublicIncludePaths.AddRange(
            new string[] 
            {
                rococoIncludeDirectory
            }
        );

        PrivateIncludePaths.AddRange(
          new string[]
          {
                rococoIncludeDirectory
          }
      );
    }
}
