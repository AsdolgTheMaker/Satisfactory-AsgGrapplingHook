#include "GrappleProjectile.h"
#include "CableComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetSystemLibrary.h"

AGrappleProjectile::AGrappleProjectile()
{
	CableComponent = CreateDefaultSubobject<UCableComponent>("CableRender");
	CableComponent->SetupAttachment(RootComponent);
	
	mShouldAttachOnImpact = true;
}

void AGrappleProjectile::SetFirstPersonCableMaterial()
{
	CableComponent->SetMaterial(0, CableMaterial1P);
	bWasMaterial1P = true;
}

void AGrappleProjectile::BeginPlay()
{
	Super::BeginPlay();

	CableComponent->EndLocation = FVector::ZeroVector;
	CableComponent->SetMaterial(0, bWasMaterial1P ? CableMaterial1P : CableMaterial);
}

void AGrappleProjectile::OnImpact_Native(const FHitResult& HitResult)
{
	Super::OnImpact_Native(HitResult);
	
	OnProjectileImpactEvent.Broadcast(HitResult);
}
