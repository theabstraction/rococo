// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;

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
    }
}
