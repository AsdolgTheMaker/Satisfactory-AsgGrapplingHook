#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrappleProjectile.generated.h"

class UCableComponent;
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogAsgGrapple, Log, All);

UCLASS(Abstract)
class ASGGRAPPLINGHOOK_API AGrappleProjectile : public AActor
{
	GENERATED_BODY()

public:
	AGrappleProjectile();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCableComponent> CableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> CableMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> CableMaterial1P;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FlightGravityScale = 1.f;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFGOnProjectileImpact, const FHitResult&, HitResult);
	FFGOnProjectileImpact OnProjectileImpactEvent;

public:
	void SetInitialVelocity(const FVector& InVelocity);
	void SetFirstPersonCableMaterial();

	void ConfigureAsSimulated(AActor* InInstigatorActor);
	void ConfigureAsCosmetic();

	void AttachToAnchor(const FVector& Location, const FRotator& Rotation, USceneComponent* AttachTo = nullptr, FName BoneName = NAME_None);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

private:
	bool bWasMaterial1P = false;
	bool bCosmetic = false;
	bool bImpacted = false;
};
