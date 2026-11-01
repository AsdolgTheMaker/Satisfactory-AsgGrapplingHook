#include "GrappleLocalConfig.h"

#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "HAL/IConsoleManager.h"

FString UGrappleLocalConfig::ConfigFilePath()
{
	// Normalized so it matches the key GConfig caches the file under.
	return FConfigCacheIni::NormalizeConfigIniPath(
		FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Config"), TEXT("AsgGrapplingHook.ini")));
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
	const FString Path = ConfigFilePath();
	// LoadConfig pulls an existing ini into GConfig through FConfigCacheIni::Find, which flags it NoSave,
	// so SaveConfig would update the cache but never write the file. Drop the cached copy so SaveConfig
	// writes through a temporary branch instead.
	GConfig->Remove(Path);
	SaveConfig(CPF_Config, *Path);
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
