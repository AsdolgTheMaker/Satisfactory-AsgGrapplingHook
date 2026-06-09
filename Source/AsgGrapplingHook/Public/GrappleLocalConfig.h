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

	UPROPERTY(Config)
	int32 ControlsHintReelSuccesses = 0;
	UPROPERTY(Config)
	int32 ControlsHintReelOutSuccesses = 0;
	UPROPERTY(Config)
	int64 ControlsHintLastPlayUtcTicks = 0;

	static constexpr int32 ControlsHintReelInTarget = 5;
	static constexpr int32 ControlsHintReelOutTarget = 3;

	static UGrappleLocalConfig* Get();
	void Persist();

private:
	static FString ConfigFilePath();
};
