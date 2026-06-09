#include "GrappleLocalConfig.h"

#include "Misc/Paths.h"

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
