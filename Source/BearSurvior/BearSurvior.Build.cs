// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BearSurvior : ModuleRules
{
	public BearSurvior(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate", 
			"SlateCore", 
		});

		PublicIncludePaths.AddRange(new string[] {
			"BearSurvior",
			"BearSurvior/Variant_Platforming",
			"BearSurvior/Variant_Combat",
			"BearSurvior/Variant_Combat/AI",
			"BearSurvior/Variant_SideScrolling",
			"BearSurvior/Variant_SideScrolling/Gameplay",
			"BearSurvior/Variant_SideScrolling/AI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
