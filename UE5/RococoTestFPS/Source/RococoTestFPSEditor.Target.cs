// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

public class RococoTestFPSEditorTarget : TargetRules
{
    private TargetInfo targetInfo;
    private string gameDir;

    private string GetConfigItem(string item, string configText, string source)
    {
        int i = configText.IndexOf(item);
        if (i < 0)
        {
            return null;
        }

        int newLineIndex = configText.IndexOf('\r', i);
        int lineFeedIndex = configText.IndexOf('\n', i);

        int endIndex = 0;

        if (newLineIndex < 0)
        {
            endIndex = lineFeedIndex;
        }
        else
        {
            if (lineFeedIndex < 0)
            {
                endIndex = newLineIndex;
            }
            else
            {
                endIndex = System.Math.Min(lineFeedIndex, newLineIndex);
            }
        }

        if (endIndex < 0)
        {
            return null;
        }

        return configText.Substring(i + item.Length, endIndex - i - item.Length);
    }

    private string GetDevContentDir(string devConfigShortName, string source)
    {
        string child = gameDir;

        while (true)
        {
            var parent = Directory.GetParent(child);
            if (parent == null)
            {
                // Root
                return null;
            }

            string devConfigFullName = Path.Combine(parent.FullName, devConfigShortName);
            if (File.Exists(devConfigFullName))
            {
                string devConfigText = File.ReadAllText(devConfigFullName);
                string devContentRelPath = GetConfigItem("Dev.Content=", devConfigText, devConfigFullName);
                if (devContentRelPath == null)
                {
                    throw new Exception("Could not find 'Dev.Content=<path-relative-to-config-file-parent>' in " + devConfigFullName);
                }

                string devContentFullPath = Path.Combine(parent.FullName, devContentRelPath);
                if (!Directory.Exists(devContentFullPath))
                {
                    throw new Exception("Could not find " + devContentFullPath + " specified in " + devConfigFullName);
                }

                return devContentFullPath;
            }

            child = parent.FullName;
        }
    }

    string[] contentDirectoriesToCopy =
    {
        "tests"
    };

    private void CopyRococoContentToUE5Content(TargetInfo target)
    {
        string rococoContentCfg = Path.Combine(gameDir, "rococo.UE5.cfg");
        if (!File.Exists(rococoContentCfg))
        {
            throw new Exception("Could not find config file " + rococoContentCfg);
        }

        string configText = File.ReadAllText(rococoContentCfg);

        string devConfigShortName = GetConfigItem("Dev.Config=", configText, rococoContentCfg);
        if (devConfigShortName == null)
        {
            return;
        }

        string devContentDir = GetDevContentDir(devConfigShortName, rococoContentCfg);

        string packageRelPath = GetConfigItem("Packaged.Content=", configText, rococoContentCfg);
        if (packageRelPath == null)
        {
            throw new Exception("Could not find Package.Content=<rococo-content-relative-to-game-dir> in " + rococoContentCfg);
        }

        string rococoContentDir = Path.Combine(gameDir, packageRelPath);
        if (!Directory.Exists(rococoContentDir))
        {
            Directory.CreateDirectory(rococoContentDir);
        }

        foreach (string subDir in contentDirectoriesToCopy)
        {
            string fullDevSubDir = Path.Combine(devContentDir, subDir);
            string fullPackageSubDir = Path.Combine(rococoContentDir, subDir);

            var devFiles = Directory.EnumerateFiles(fullDevSubDir);
            foreach (string fullDevFile in devFiles)
            {
                string devFile = Path.GetFileName(fullDevFile);
                string packageFile = Path.Combine(rococoContentDir, devFile);

                Console.WriteLine(string.Format("Copying {0} to {1}", fullDevFile, packageFile));
                File.Copy(fullDevFile, packageFile, true);
            }
        }
    }
    private void InitDirectories()
    {
        gameDir = Path.Combine(Directory.GetParent(targetInfo.ProjectFile.FullName).FullName, "Content");
        if (!Directory.Exists(gameDir))
        {
            throw new Exception("Could not find directory: " + gameDir);
        }
    }

    public RococoTestFPSEditorTarget(TargetInfo Target) : base(Target)
	{
        this.targetInfo = Target;
        InitDirectories();

        Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("RococoTestFPS");

        CopyRococoContentToUE5Content(Target);
    }
}
