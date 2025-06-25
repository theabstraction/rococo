// Copyright Epic Games, Inc. All Rights Reserved.

using System;
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
        BuildEnvironment = TargetBuildEnvironment.Unique;
        bUseLoggingInShipping = true;
        targetInfo = Target;

        InitDirectories();

        Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("RococoTestFPS");
    }
}
