#pragma once

#include "CoreMinimal.h"
#include "GrappleLocalConfig.generated.h"

UCLASS(Config = Game)
class ASGGRAPPLINGHOOK_API UGrappleLocalConfig : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Config)
	float ReelSpeedRatio = 1.f;
	UPROPERTY(Config)
	float MaxCableLengthRatio = 1.f;
	UPROPERTY(Config)
	float EjectionSpeedRatio = 1.f;

	UPROPERTY(Config)
	float ReelSpeedMaxSeen = -1.f;
	UPROPERTY(Config)
	float MaxCableLengthMaxSeen = -1.f;
	UPROPERTY(Config)
	float EjectionSpeedMaxSeen = -1.f;

	static UGrappleLocalConfig* Get() { return GetMutableDefault<UGrappleLocalConfig>(); }
};
