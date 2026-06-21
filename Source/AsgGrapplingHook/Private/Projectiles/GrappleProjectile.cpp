#include "Projectiles/GrappleProjectile.h"
#include "CableComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

DEFINE_LOG_CATEGORY(LogAsgGrapple);

AGrappleProjectile::AGrappleProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetReplicatingMovement(false);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(8.f);
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	CollisionComponent->SetGenerateOverlapEvents(false);
	CollisionComponent->CanCharacterStepUpOn = ECB_No;
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CableComponent = CreateDefaultSubobject<UCableComponent>(TEXT("CableRender"));
	CableComponent->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bSweepCollision = true;
	ProjectileMovement->bAutoActivate = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 0.f;
}

void AGrappleProjectile::BeginPlay()
{
	Super::BeginPlay();

	CableComponent->EndLocation = FVector::ZeroVector;
	if (UMaterialInterface* DesiredCableMaterial = bWasMaterial1P ? CableMaterial1P : CableMaterial)
	{
		CableComponent->SetMaterial(0, DesiredCableMaterial);
	}
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AGrappleProjectile::HandleProjectileStop);
}

void AGrappleProjectile::HandleProjectileStop(const FHitResult& ImpactResult)
{
	if (bCosmetic || bImpacted)
	{
		return;
	}
	bImpacted = true;
	OnProjectileImpactEvent.Broadcast(ImpactResult);
}

void AGrappleProjectile::SetInitialVelocity(const FVector& InVelocity)
{
	if (!ProjectileMovement)
	{
		return;
	}
	ProjectileMovement->Velocity = InVelocity;
	ProjectileMovement->InitialSpeed = InVelocity.Size();
	ProjectileMovement->Activate(true);
}

void AGrappleProjectile::SetFirstPersonCableMaterial()
{
	if (CableComponent)
	{
		CableComponent->SetMaterial(0, CableMaterial1P);
	}
	bWasMaterial1P = true;
}

void AGrappleProjectile::ConfigureAsSimulated(AActor* InInstigatorActor)
{
	bCosmetic = false;
	if (InInstigatorActor && CollisionComponent)
	{
		CollisionComponent->IgnoreActorWhenMoving(InInstigatorActor, true);
	}
}

void AGrappleProjectile::ConfigureAsCosmetic()
{
	bCosmetic = true;
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AGrappleProjectile::AttachToAnchor(const FVector& Location, const FRotator& Rotation, USceneComponent* AttachTo, const FName BoneName)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (USceneComponent* Root = GetRootComponent(); Root && Root->GetAttachParent())
	{
		Root->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
	SetActorLocationAndRotation(Location, Rotation);
	if (AttachTo)
	{
		GetRootComponent()->AttachToComponent(AttachTo, FAttachmentTransformRules::KeepWorldTransform, BoneName);
	}
}
