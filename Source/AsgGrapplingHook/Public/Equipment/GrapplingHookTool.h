#pragma once

#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "Equipment/FGWeapon.h"
#include "Projectiles/GrappleProjectile.h"
#include "GrapplingHookTool.generated.h"

class AGrapplingHookTool;
class UFGCharacterMovementComponent;
class UFGInputMappingContext;
class AFGPlayerController;
class UFGEnhancedInputComponent;
class AFGProjectile;

UCLASS()
class ASGGRAPPLINGHOOK_API UGrapplingHookRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

public:
	UFUNCTION(Server, Reliable)
	void ServerShootGrapple(AGrapplingHookTool* Tool, const FVector& ShootingSourceLocation, const FVector& PlayerAimDirection);
	UFUNCTION(Server, Reliable)
	void ServerRetractGrapple(AGrapplingHookTool* Tool);
	
	UFUNCTION(Server, Reliable)
	void ServerProcessDesiredCableLengthQueries(AGrapplingHookTool* Tool, int32 Queries, float DeltaSeconds);

private:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// At least one property should be replicated in order for RCO to work.
	UPROPERTY(Replicated)
	bool bDummy = true;
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

	UFUNCTION(Server, Unreliable)
	void ServerTickGrapple(float DeltaSeconds);
	UFUNCTION()
	void ClientTickGrapple(float DeltaSeconds);
	
	UFUNCTION(NetMulticast, Unreliable)
	void SetCableLength(float NewLength);
	
	UFUNCTION(NetMulticast, Unreliable)
	void SetInstigatorVelocity(const FVector& NewVelocity);
	
	UFUNCTION()
	void HandleInput_PrimaryFire();
	UFUNCTION()
	void HandleInput_RetractCable();	
	UFUNCTION()
	void HandleInput_ExtendCable();

	void TickTensionForce(float DeltaSeconds, bool bPropagateOverNetwork);

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
	int32 GetDesiredCableLengthQueries() const;

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
	UFGInputMappingContext* GrappleInputContext = nullptr; 
	// Action to shoot/return grapple projectile.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	UInputAction* PrimaryFireAction = nullptr;
	// Action to shrink cable's desired length.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	UInputAction* RetractCableAction = nullptr;
	// Action to prolong cable's desired length.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Input)
	UInputAction* ExtendCableAction = nullptr;

	// Widget that will appear on screen when player is aiming at reachable surface.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple")
	TSubclassOf<UUserWidget> CrosshairHighlightWidgetClass;
	
	// Projectile that will be shot from the tool.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Projectile")
	TSubclassOf<AGrappleProjectile> GrappleProjectileClass = nullptr;
	// Speed at which grapple projectile will be shot.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Projectile")
	float InitialHookVelocity = 6000.0f;
	
	// Socket name to which cable's end will be attached. Target component defined by GetCableAttachComponent implementation.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Cable")
	FName CableAttachComponentSocket;
	// Max allowed cable's desired length. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Cable")
	float MaxCableLength = 6000;
	// Cable length in cm/s that would be subtracted/added from/to desired length by respective inputs.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Cable")
	float CableLengthControlStep = 500;
	// Cable gravity multiplier applied after projectile was shot but it didnt hit anything yet. 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Cable")
	float CableGravityScaleBeforeHit = 0;
	// Cable gravity multiplier applied after projectile hits something.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grapple|Cable")
	float CableGravityScaleAfterHit = 3;

private:
	// Turn grapple-specific inputs on/off 
	void SetInputContextRegistered(AFGPlayerController* Controller, const bool bRegistered);

	UFUNCTION()
	void OnRep_GrappleProjectile();
	UFUNCTION()
	void OnRep_GrappleAttached();
	UFUNCTION()
	void OnRep_DesiredCableLength();
	
private:
	// Grapple projectile that was shot from this tool.
	UPROPERTY(Transient, ReplicatedUsing=OnRep_GrappleProjectile)
	AGrappleProjectile* GrappleProjectile = nullptr;

	// Desired length is length at which tension force will be applied to player.
	// When it is larger than actual length, tension is not applied.
	// When it is smaller than actual length, additional pulling force is applied.  
	UPROPERTY(Transient, ReplicatedUsing=OnRep_DesiredCableLength)
	float DesiredCableLength = 0;

	// Stacked length control inputs; will be processed and zerofied at Tick.
	int32 DesiredCableLengthControlQuery = 0;

	// Whether grapple projectile is sticked to something.
	UPROPERTY(Transient, ReplicatedUsing=OnRep_GrappleAttached)
	bool bGrappleAttached = false;

	// Whether input actions were bound to their respective delegates
	bool bInputsBound = false;

	// Local flag indicating whether projectile should be located inside the tool or not.
	bool bRetracted = true;

	UPROPERTY(Transient)
	UUserWidget* CrosshairHighlightWidget;
};
