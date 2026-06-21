#pragma once

#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "Equipment/FGWeapon.h"
#include "Input/FGBoundMappingContextHandle.h"
#include "Projectiles/GrappleProjectile.h"
#include "GrapplingHookTool.generated.h"

class AGrapplingHookTool;
class UFGCharacterMovementComponent;
class UFGInputMappingContext;
class AFGPlayerController;
class UFGEnhancedInputComponent;
class UGrappleControlsHintWidget;

UCLASS()
class ASGGRAPPLINGHOOK_API UGrapplingHookRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	UFUNCTION(Server, Reliable)
	void ServerReportGrappleFired(AGrapplingHookTool* Tool, const FVector& FireLocation, const FVector& FireVelocity);

	UFUNCTION(Server, Reliable)
	void ServerReportGrappleAttached(AGrapplingHookTool* Tool, const FVector& AnchorLocation, const FRotator& AnchorRotation, float DesiredLength);

	UFUNCTION(Server, Reliable)
	void ServerReportGrappleRetracted(AGrapplingHookTool* Tool, const FVector& ReleaseVelocity);

	UFUNCTION(Server, Reliable)
	void ServerSetDesiredCableLength(AGrapplingHookTool* Tool, float NewLength);

	UFUNCTION(Server, Unreliable)
	void ServerReportGrappleVelocity(AGrapplingHookTool* Tool, const FVector& Velocity);

	UFUNCTION(Server, Reliable)
	void ServerSetGrappleConfig(AGrapplingHookTool* Tool, float ReelSpeedRatio, float MaxCableLengthRatio, float EjectionSpeedRatio);

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(Replicated)
	bool bDummy = true;
};

USTRUCT(BlueprintType)
struct FGrapplingHookUpgradesChain
{
	GENERATED_BODY()

public:
	float GetActiveValue(const UObject* Context) const;
	float GetActiveValue(UWorld* Context) const;

	float GetMaximumValue(const UObject* Context) const;
	float GetMaximumValue(UWorld* Context) const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseValue = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSubclassOf<UFGSchematic>, float> Upgrades;

	UPROPERTY(BlueprintReadOnly)
	float ConfigRatio = 1.f;
};

UCLASS(Abstract)
class ASGGRAPPLINGHOOK_API AGrapplingHookTool : public AFGEquipment
{
	GENERATED_BODY()

	friend class UGrapplingHookRCO;

public:
	AGrapplingHookTool();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void LoadFromItemState_Implementation(const FFGDynamicStruct& ItemState) override;
	virtual FFGDynamicStruct SaveToItemState_Implementation() const override;
	virtual bool ShouldSave_Implementation() const override;

	virtual void Equip(AFGCharacterPlayer* Character) override;
	virtual void UnEquip() override;
	virtual void AddEquipmentActionBindings() override;

	void RetractGrapple();

	UFUNCTION(BlueprintCallable, Category="Grapple|Config")
	void SetGrappleConfig(float NewReelSpeedRatio, float NewMaxCableLengthRatio, float NewEjectionSpeedRatio);

	UFUNCTION(BlueprintCallable, Category="Grapple|Config")
	void RefreshGrappleConfig();

	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetReelSpeedRatio() const { return UpgradesSpeed.ConfigRatio; }
	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetMaxCableLengthRatio() const { return UpgradesLength.ConfigRatio; }
	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetEjectionSpeedRatio() const { return UpgradesPower.ConfigRatio; }

	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetReelSpeedValue() const { return UpgradesSpeed.GetActiveValue(this); }
	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetMaxCableLengthValue() const { return UpgradesLength.GetActiveValue(this); }
	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetEjectionSpeedValue() const { return UpgradesPower.GetActiveValue(this); }

	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetReelSpeedMaximum() const { return UpgradesSpeed.GetMaximumValue(this); }
	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetMaxCableLengthMaximum() const { return UpgradesLength.GetMaximumValue(this); }
	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetEjectionSpeedMaximum() const { return UpgradesPower.GetMaximumValue(this); }

	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetReelSpeedBase() const { return UpgradesSpeed.BaseValue; }
	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetMaxCableLengthBase() const { return UpgradesLength.BaseValue; }
	UFUNCTION(BlueprintPure, Category="Grapple|Config") float GetEjectionSpeedBase() const { return UpgradesPower.BaseValue; }

	UFUNCTION(BlueprintPure, Category="Grapple|Config") bool IsReelSpeedTunable() const { return UpgradesSpeed.GetMaximumValue(this) > UpgradesSpeed.BaseValue + KINDA_SMALL_NUMBER; }
	UFUNCTION(BlueprintPure, Category="Grapple|Config") bool IsMaxCableLengthTunable() const { return UpgradesLength.GetMaximumValue(this) > UpgradesLength.BaseValue + KINDA_SMALL_NUMBER; }
	UFUNCTION(BlueprintPure, Category="Grapple|Config") bool IsEjectionSpeedTunable() const { return UpgradesPower.GetMaximumValue(this) > UpgradesPower.BaseValue + KINDA_SMALL_NUMBER; }

	UFUNCTION(BlueprintPure, Category="Grapple|Hint") bool ShouldShowControlsHint() const;

	UFUNCTION(Server, Unreliable)
	void ServerTickGrapple(float DeltaSeconds);
	UFUNCTION()
	void ClientTickGrapple(float DeltaSeconds);

	UFUNCTION()
	void HandleInput_PrimaryFire();
	UFUNCTION()
	void HandleInput_RetractCable();
	UFUNCTION()
	void HandleInput_ExtendCable();

	void TickTensionForce(float DeltaSeconds);

	void UpdateDeathState();

protected:
	UFUNCTION()
	void OnGrappleHitSurface(const FHitResult& HitResult);

	float GetDistanceToGrappleForcePoint() const;
	UFUNCTION(BlueprintPure)
	FVector GetGrappleForcePoint() const;

	UFUNCTION(BlueprintPure)
	float GetDesiredCableLengthQueries() const;

	UFUNCTION(BlueprintImplementableEvent)
	void OnGrappleFired();
	UFUNCTION(BlueprintImplementableEvent)
	void OnGrappleAttached();
	UFUNCTION(BlueprintImplementableEvent)
	void OnGrappleStartedRetracting();
	UFUNCTION(BlueprintImplementableEvent)
	void OnGrappleFinishedRetracting();

	UFUNCTION(BlueprintImplementableEvent)
	void OnDesiredCableLengthChanged(float NewRatio);

	float GetCableLengthControlStep() const;
	float GetMaxCableLength() const;
	float GetTearingDistance() const;
	float GetInitialHookVelocity() const;

	UFUNCTION(BlueprintNativeEvent)
	USceneComponent* GetCableAttachComponent() const;
	USceneComponent* GetCableAttachComponent_Implementation() const;
	UFUNCTION(BlueprintImplementableEvent)
	FVector GetShootingSourceLocation() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	TObjectPtr<UFGInputMappingContext> GrappleInputContext = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	TObjectPtr<UInputAction> PrimaryFireAction = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	TObjectPtr<UInputAction> RetractCableAction = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	TObjectPtr<UInputAction> ExtendCableAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple")
	TSubclassOf<UUserWidget> CrosshairHighlightWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple")
	TSubclassOf<UGrappleControlsHintWidget> ControlsHintWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Projectile")
	TSubclassOf<AGrappleProjectile> GrappleProjectileClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Upgrades")
	FGrapplingHookUpgradesChain UpgradesLength;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Upgrades")
	FGrapplingHookUpgradesChain UpgradesPower;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Upgrades")
	FGrapplingHookUpgradesChain UpgradesSpeed;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Upgrades")
	FGrapplingHookUpgradesChain UpgradesDurability;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Cable")
	FName CableAttachComponentSocket;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Cable")
	float CableGravityScaleBeforeHit = 0;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Cable")
	float CableGravityScaleAfterHit = 3;

private:
	void SetInputContextRegistered(AFGPlayerController* Controller, const bool bRegistered);

	void SetClientAuthoritativeMovement(bool bEnable);

	void ApplyReleaseVelocity(const FVector& ReleaseVelocity);

	void SpawnSimulatedProjectile(const FVector& FireLocation, const FVector& FireVelocity);
	void SpawnCosmeticProjectile(const FVector& FireLocation, const FVector& FireVelocity);
	void DestroyProjectile();

	void ReassertOwnerState();

	void UpdateCableVisual();

	FString GrappleNetContext() const;

	UFUNCTION()
	void OnRep_ProjectileActive();
	UFUNCTION()
	void OnRep_GrappleAttached();
	UFUNCTION()
	void OnRep_DesiredCableLength();
	UFUNCTION()
	void OnRep_GrappleForcePointLocation();

private:

	UPROPERTY(Transient, ReplicatedUsing=OnRep_ProjectileActive)
	bool bGrappleProjectileActive = false;

	UPROPERTY(Transient, Replicated)
	FVector GrappleFireLocation = FVector::ZeroVector;
	UPROPERTY(Transient, Replicated)
	FVector GrappleFireVelocity = FVector::ZeroVector;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_DesiredCableLength)
	float DesiredCableLength = 0;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_GrappleAttached)
	bool bGrappleAttached = false;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_GrappleForcePointLocation)
	FVector GrappleForcePointLocation = FVector::ZeroVector;

	UPROPERTY(Transient, Replicated)
	FRotator GrappleForcePointRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient, Replicated)
	float ReplicatedReelSpeedMax = -1.f;

	bool bLocalProjectileActive = false;
	bool bLocalAttached = false;
	bool bAnchorIsDynamic = false;
	FVector LocalAnchorLocation = FVector::ZeroVector;
	FRotator LocalAnchorRotation = FRotator::ZeroRotator;
	float LocalDesiredCableLength = 0.f;
	FVector LocalFireLocation = FVector::ZeroVector;
	FVector LocalFireVelocity = FVector::ZeroVector;
	float LocalFlightTime = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<AGrappleProjectile> GrappleProjectile = nullptr;

	float DesiredCableLengthControlQuery = 0;

	bool bInputsBound = false;

	FBoundMappingContextHandle GrappleInputContextHandle;

	bool bRetracted = true;

	bool bOwnerDead = false;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CrosshairHighlightWidget;

	UPROPERTY(Transient)
	TObjectPtr<UGrappleControlsHintWidget> ControlsHintWidget;

	void ApplyConfigRatios();
	void PushConfigToServer();

	bool ReconcileResearchSnap();

	float ConfigSnapCheckAccumulator = 0.f;

	void ReconcileControlsHintMemory();
	void RegisterReelSuccess();
	void RegisterReelOutSuccess();

	bool bWasAttachedForHint = false;
	bool bControlsHintVisible = false;
	bool bReelInputThisFrame = false;
	float ReelHoldSeconds = 0.f;
	bool bReelHoldCounted = false;
	bool bExtendInputThisFrame = false;
	float ExtendHoldSeconds = 0.f;
	bool bExtendHoldCounted = false;
};
