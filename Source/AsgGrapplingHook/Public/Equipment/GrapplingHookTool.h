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
class AFGProjectile;
class UGrappleControlsHintWidget;

UCLASS()
class ASGGRAPPLINGHOOK_API UGrapplingHookRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	UFUNCTION(Server, Reliable)
	void ServerShootGrapple(AGrapplingHookTool* Tool, const FVector& ShootingSourceLocation, const FVector& PlayerAimDirection);
	UFUNCTION(Server, Reliable)
	void ServerRetractGrapple(AGrapplingHookTool* Tool, const FVector& ReleaseVelocity);
	
	UFUNCTION(Server, Reliable)
	void ServerProcessDesiredCableLengthQueries(AGrapplingHookTool* Tool, float QueriedChange, float DeltaSeconds);

	// Applies client's config to server-driven physics
	UFUNCTION(Server, Reliable)
	void ServerSetGrappleConfig(AGrapplingHookTool* Tool, float ReelSpeedRatio, float MaxCableLengthRatio, float EjectionSpeedRatio);

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// At least one property should be replicated in order for RCO to work.
	UPROPERTY(Replicated)
	bool bDummy = true;
};

USTRUCT(BlueprintType)
struct FGrapplingHookUpgradesChain
{
	GENERATED_BODY()

public:
	// User's preferred value
	float GetActiveValue(const UObject* Context) const;
	float GetActiveValue(UWorld* Context) const;

	// Researched max value
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
	
	//~ Begin AActor interface
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AActor interface

	//~ Begin AFGEquipment interface
	virtual void LoadFromItemState_Implementation(const FFGDynamicStruct& ItemState) override;
	virtual FFGDynamicStruct SaveToItemState_Implementation() const override;
	virtual bool ShouldSave_Implementation() const override;
	
	virtual void Equip(AFGCharacterPlayer* Character) override;
	virtual void UnEquip() override;
	virtual void AddEquipmentActionBindings() override;
	//~ End AFGEquipment interface

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
	
	UFUNCTION(NetMulticast, Unreliable)
	void SetCableLength(float NewLength);

	UFUNCTION()
	void HandleInput_PrimaryFire();
	UFUNCTION()
	void HandleInput_RetractCable();	
	UFUNCTION()
	void HandleInput_ExtendCable();

	void TickTensionForce(float DeltaSeconds);

protected:
	// Called when grapple projectile hits something (bound to respective event in AGrappleProjectile)
	UFUNCTION()
	void OnGrappleHitSurface(const FHitResult& HitResult);

	// Returns actual distance along cable geometry from player to grapple point.
	float GetDistanceToGrappleForcePoint() const;
	// Returns point towards which tension force will be applied.
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
	// Tuned/active max cable length (respects the player's config), not the researched ceiling.
	float GetMaxCableLength() const;
	float GetTearingDistance() const;
	float GetInitialHookVelocity() const;

	// Returns component to which cable's end should be attached. Optionally can provide a socket with CableAttachComponentSocket property. 
	UFUNCTION(BlueprintNativeEvent)
	USceneComponent* GetCableAttachComponent() const;
	USceneComponent* GetCableAttachComponent_Implementation() const;
	// Returns point from which projectiles will be shot.
	UFUNCTION(BlueprintImplementableEvent)
	FVector GetShootingSourceLocation() const;

protected:	
	// Input context to register when grapple tool is equipped (will unregister when unequipped).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	TObjectPtr<UFGInputMappingContext> GrappleInputContext = nullptr; 
	// Action to shoot/return grapple projectile.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	TObjectPtr<UInputAction> PrimaryFireAction = nullptr;
	// Action to shrink cable's desired length.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	TObjectPtr<UInputAction> RetractCableAction = nullptr;
	// Action to prolong cable's desired length.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	TObjectPtr<UInputAction> ExtendCableAction = nullptr;

	// Widget that will appear on screen when player is aiming at reachable surface.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple")
	TSubclassOf<UUserWidget> CrosshairHighlightWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple")
	TSubclassOf<UGrappleControlsHintWidget> ControlsHintWidgetClass;
	
	// Projectile that will be shot from the tool.
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
	
	// Socket name to which cable's end will be attached. Target component defined by GetCableAttachComponent implementation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Cable")
	FName CableAttachComponentSocket;
	// Cable gravity multiplier applied after projectile was shot but it didnt hit anything yet. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Cable")
	float CableGravityScaleBeforeHit = 0;
	// Cable gravity multiplier applied after projectile hits something.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Cable")
	float CableGravityScaleAfterHit = 3;

private:
	// Turn grapple-specific inputs on/off
	void SetInputContextRegistered(AFGPlayerController* Controller, const bool bRegistered);

	void SetClientAuthoritativeMovement(bool bEnable);

	void ApplyReleaseVelocity(const FVector& ReleaseVelocity);

	UFUNCTION()
	void OnRep_GrappleProjectile();
	UFUNCTION()
	void OnRep_GrappleAttached();
	UFUNCTION()
	void OnRep_DesiredCableLength();
	UFUNCTION()
	void OnRep_GrappleForcePointLocation();
	
private:
	// Grapple projectile that was shot from this tool.
	UPROPERTY(Transient, ReplicatedUsing=OnRep_GrappleProjectile)
	TObjectPtr<AGrappleProjectile> GrappleProjectile = nullptr;

	// Desired length is length at which tension force will be applied to player.
	// When it is larger than actual length, tension is not applied.
	// When it is smaller than actual length, additional pulling force is applied.  
	UPROPERTY(Transient, ReplicatedUsing=OnRep_DesiredCableLength)
	float DesiredCableLength = 0;

	// Stacked length control inputs; will be processed and zerofied at Tick.
	float DesiredCableLengthControlQuery = 0;

	// Whether grapple projectile is sticked to something.
	UPROPERTY(Transient, ReplicatedUsing=OnRep_GrappleAttached)
	bool bGrappleAttached = false;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_GrappleForcePointLocation)
	FVector GrappleForcePointLocation = FVector::ZeroVector;

	UPROPERTY(Transient, Replicated)
	FRotator GrappleForcePointRotation = FRotator::ZeroRotator;

	// Whether input actions were bound to their respective delegates
	bool bInputsBound = false;

	// Handle tracking the grapple input mapping context binding on the player controller
	// (5.6 Enhanced Input replaced AFGPlayerController::SetMappingContextBound with a handle-based API)
	FBoundMappingContextHandle GrappleInputContextHandle;

	// Local flag indicating whether projectile should be located inside the tool or not.
	bool bRetracted = true;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CrosshairHighlightWidget;

	UPROPERTY(Transient)
	TObjectPtr<UGrappleControlsHintWidget> ControlsHintWidget;

	void ApplyConfigRatios();
	void PushConfigToServer();
	
	// Snaps a stat back to max when its researched ceiling grew since last set. Returns true if anything changed.
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
