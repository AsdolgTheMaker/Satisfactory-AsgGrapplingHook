#pragma once

#include "CoreMinimal.h"
#include "FGProjectile.h"
#include "GrappleProjectile.generated.h"

class UCableComponent;

UCLASS(Abstract)
class ASGGRAPPLINGHOOK_API AGrappleProjectile : public AFGProjectile
{
	GENERATED_BODY()

public:
	AGrappleProjectile();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCableComponent* CableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UMaterialInterface* CableMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UMaterialInterface* CableMaterial1P;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFGOnProjectileImpact, const FHitResult&, HitResult);
	FFGOnProjectileImpact OnProjectileImpactEvent;

public:
	void SetFirstPersonCableMaterial();
	
protected:
	//~ Begin AActor interface
	virtual void BeginPlay() override;
	//~ End AActor interface
	
	//~ Begin AFGProjectile interface
	virtual void OnImpact_Native(const FHitResult& HitResult) override;
	//~ End AFGProjectile interface

private:
	bool bWasMaterial1P = false;
};
