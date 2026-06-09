#include "GrappleLocalConfig.h"

#include "Misc/Paths.h"
#include "HAL/IConsoleManager.h"

FString UGrappleLocalConfig::ConfigFilePath()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Config"), TEXT("AsgGrapplingHook.ini"));
}

UGrappleLocalConfig* UGrappleLocalConfig::Get()
{
	UGrappleLocalConfig* Config = GetMutableDefault<UGrappleLocalConfig>();
	static bool bLoaded = false;
	if (!bLoaded)
	{
		bLoaded = true;
		Config->LoadConfig(UGrappleLocalConfig::StaticClass(), *ConfigFilePath());
	}
	return Config;
}

void UGrappleLocalConfig::Persist()
{
	SaveConfig(CPF_Config, *ConfigFilePath());
}

static FAutoConsoleCommand GClearGrappleTutorialCommand(
	TEXT("Grapple.ClearTutorial"),
	TEXT("Resets the grappling hook controls-hint tutorial so it appears again."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UGrappleLocalConfig* Config = UGrappleLocalConfig::Get();
		Config->ControlsHintReelSuccesses = 0;
		Config->ControlsHintReelOutSuccesses = 0;
		Config->ControlsHintLastPlayUtcTicks = 0;
		Config->Persist();
	}));
