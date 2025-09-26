// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnrealBuildTool;

public class RococoTestFPSTarget : TargetRules
{
    public RococoTestFPSTarget(TargetInfo Target) : base(Target)
	{
        BuildEnvironment = TargetBuildEnvironment.Unique;
        bUseLoggingInShipping = true;
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("RococoTestFPS");
		// PreBuildSteps.Add("copy-rococo-files.bat");
    }
}
