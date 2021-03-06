// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class KrazyKartsEditorTarget : TargetRules
{
	public KrazyKartsEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
	    bUseUnityBuild = false;
	    bUsePCHFiles = false;
		ExtraModuleNames.Add("KrazyKarts");
	}
}
