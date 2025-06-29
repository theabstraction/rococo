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
    private string rococoIncludeDirectory;
	private string rococoSexyIncludeDirectory;
	private string rococoHomeDirectory;
    private string rococoSourceDirectory;
	private string thisSourceDirectory;
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
        rococoSexyIncludeDirectory = Path.Combine(rococoHomeDirectory, "source/rococo/sexy/Common/").Replace('/', Path.DirectorySeparatorChar);
        rococoSourceDirectory = Path.Combine(_rococoHomeDirectory, "source").Replace('/', Path.DirectorySeparatorChar);
    }

    private string PrivateSourceRelPath
    {
        get 
        {
            return "Source/RococoUtil/Private".Replace('/', Path.DirectorySeparatorChar);
        }
    }

    private void PrepRococoDirectories()
    {
        if (rococoIncludeDirectory == null)
        {
            string dir = PluginDirectory;
            string fullPath = dir;
            string lastFullPath;

            thisSourceDirectory = Path.Combine(PluginDirectory, PrivateSourceRelPath);
            if (!Directory.Exists(thisSourceDirectory))
            {
                throw new System.Exception("Expecting directory to exist: " + thisSourceDirectory);
            }

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

    private void CreateBundleDirect(string bundleName, string headerFile, string prelude, string postlude, string sourceDirectory, List<string> sourceNames)
    {
		StringBuilder sb = new StringBuilder();

		sb.AppendLine("// Bundle generated by Rococo.Util.Build.cs");
		sb.AppendLine("// Created: " + DateTime.UtcNow.ToString("d MMM yyyy HH:mm:ss") + " UTC");
		sb.AppendLine();

		if (headerFile != null)
		{
			sb.AppendLine("#include \"" + headerFile + "\"");
        }

		if (prelude != null)
		{
			sb.AppendLine(File.ReadAllText(Path.Combine(thisSourceDirectory, prelude)));
		}

        foreach (var sourceName in sourceNames)
		{
			string fullPath = Path.Combine(rococoSourceDirectory, sourceDirectory, sourceName);
            fullPath = fullPath.Replace("\\", "/");
            if (!File.Exists(fullPath))
			{
				throw new System.Exception("Could not find bundle file " + fullPath);
			}

			sb.AppendFormat("#include <{0}>", fullPath);
			sb.AppendLine();
		}

        if (postlude != null)
        {
            sb.AppendLine(File.ReadAllText(Path.Combine(thisSourceDirectory, postlude)));
        }

        string fullBundlePath = Path.Combine(thisSourceDirectory, bundleName);
		if (!fullBundlePath.EndsWith(".rococo-bundle.cpp"))
		{
			throw new System.Exception("Expecting bundle file to end with .rococo-bundle.cpp");
		}

		if (File.Exists(fullBundlePath))
		{
			string existingText = File.ReadAllText(fullBundlePath);
			if (sb.ToString() != existingText)
			{
				File.WriteAllText(fullBundlePath, sb.ToString());
			}
		}
		else
		{
            File.WriteAllText(fullBundlePath, sb.ToString());
        }
    }

    private void CreateBundleByMatch(string bundleName, string prelude, string postlude, string sourceDirectory, List<string> matchPatterns)
    {
		string fullSrcPath = Path.Combine(rococoSourceDirectory, sourceDirectory);

		var files = new System.Collections.Generic.HashSet<string>();

        foreach (var mp in matchPatterns)
        {
			foreach(var file in Directory.EnumerateFiles(fullSrcPath, mp))
			{
				files.Add(file);
			}
        }

		List<string> fileList = new List<string>();

		foreach(var file in files)
		{
            fileList.Add(file);
		}

		CreateBundleDirect(bundleName, null, prelude, postlude, sourceDirectory, fileList);
    }

    private void CreateBundles()
	{
		CreateBundleDirect("rococo.s-parser.rococo-bundle.cpp", "rococo.UE5.h", "rococo.UE5.prelude.h", "rococo.UE5.postlude.h", "rococo/sexy/SP/sexy.s-parser",
			new List<string>()
			{
				"sexy.s-parser.cpp",
				"sexy.s-builder.cpp",
				"sexy.s-parser.s-block.cpp"
            }
		);

        CreateBundleDirect("rococo.s-utils.rococo-bundle.cpp", "rococo.UE5.h", null, null, "rococo/sexy/Utilities",
            new List<string>()
            {
                "sexy.util.cpp"
            }
        );

        CreateBundleDirect("rococo.sexml.rococo-bundle.cpp", "rococo.UE5.h", null, null, "rococo/rococo.sexml",
          new List<string>()
          {
                "rococo.sexml.builder.cpp",
                "rococo.sexml.parser.cpp",
                "rococo.sexml.user.cpp"
          }
		);

        CreateBundleByMatch("rococo.gui-retained.rococo-bundle.cpp", "rococo.UE5.prelude.h", "rococo.UE5.postlude.h", "rococo/rococo.gui.retained",
          new List<string>()
          {
                "rococo.gr.*.cpp"
          }
        );

        CreateBundleDirect("rococo.maths.rococo-bundle.cpp", "rococo.UE5.h", null, null, "rococo/rococo.maths",
          new List<string>()
          {
                "rococo.integer.formatting.cpp",
				"rococo.maths.cpp",
                "rococo.collisions.cpp",
          }
        );

        CreateBundleDirect("rococo.greatsex.rococo-bundle.cpp", "rococo.UE5.h", null, null, "rococo/rococo.great.sex",
            new List<string>()
            {
                "great.sex.colour.cpp",
                "great.sex.scheme.cpp",
                "great.sex.main.cpp",
                "great.sex.test-data.cpp"
            }
        );
    }

    public string RococoIncludeDirectory
	{
		get { return rococoIncludeDirectory; }
	}

    public string SexyIncludeDirectory
    {
        get { return rococoSexyIncludeDirectory; }
    }

    public string RococoHomeDirectory
    {
        get { return rococoHomeDirectory; }
    }

    public RococoUtil(ReadOnlyTargetRules Target) : base(Target)
	{
        PrepRococoDirectories();
		CreateBundles();

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bEnableExceptions = true;

        if (Target.LinkType == TargetLinkType.Monolithic)
        {
            PublicDefinitions.Add("ROCOCO_BUILD_IS_MONOLITHIC");
        }


        PublicIncludePaths.AddRange(
			new string[] {
                RococoIncludeDirectory,
                SexyIncludeDirectory
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
