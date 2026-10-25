// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class PestiAlvilag : ModuleRules
{
	public PestiAlvilag(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "ProceduralMeshComponent", "Json" });

		// The food menus are drawn with Slate.
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// The car engine sound is made while the game runs (USynthComponent).
		PrivateDependencyModuleNames.Add("AudioMixer");
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
