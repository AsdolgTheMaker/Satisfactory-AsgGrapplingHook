#include "GrapplingHookTool.h"

#include "CableComponent.h"
#include "EnhancedInputSubsystems.h"
#include "FGEnhancedInputComponent.h"
#include "FGInputMappingContext.h"
#include "FGPlayerController.h"
#include "SessionSettingsManager.h"
#include "UnrealNetwork.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#define BIND_ACTION(Controller, Action, TriggerEvent, FuncName) \
	{ \
		FFGRuntimeInputActionDelegate Delegate; \
		Delegate.BindUFunction(this, FuncName); \
		Controller->BindToAction(Action, ETriggerEvent::TriggerEvent, Delegate); \
	}

FVector2f PaniniProjection(const FVector2f OM, float d, float s)
{
	const float PaniniDirectionXZInvLength = 1.0f / FMath::Sqrt(1.0f + OM.X * OM.X);
	const float SinPhi = OM.X * PaniniDirectionXZInvLength;
	const float TanTheta = OM.Y * PaniniDirectionXZInvLength;
	const float CosPhi = FMath::Sqrt(1.0f - SinPhi * SinPhi);
	const float S = (d + 1.0f) / (d + CosPhi);

	return S * FVector2f(SinPhi, FMath::Lerp(TanTheta, TanTheta / CosPhi, s));
}

void UGrapplingHookRCO::ServerShootGrapple_Implementation(AGrapplingHookTool* Tool, const FVector& ShootingSourceLocation, const FVector& PlayerAimDirection)
{
	if (!Tool)
	{
		return;
	}
	
	if (!Tool->GrappleProjectile)
	{
		FVector SourceLocation = ShootingSourceLocation;
		FCollisionQueryParams QueryParams;
		AFGCharacterPlayer* Player = Tool->GetInstigatorCharacter();
		QueryParams.AddIgnoredActor(Player);
		
		if (FHitResult HitResult;
			GetWorld()->LineTraceSingleByProfile(HitResult,
				ShootingSourceLocation,
				ShootingSourceLocation + PlayerAimDirection * 150,
				"Projectile", QueryParams))
		{
			SourceLocation = HitResult.Location; 
		}
		else
		{
			SourceLocation = ShootingSourceLocation + PlayerAimDirection * 150;
		}

		// Take player's velocity projected on shooting vector, so the projectile inherits player speed in that direction (so we never have a situation when player is faster than the projectile, unless terminal velocity is involved) 
		const FVector PlayerVelocity = Player->GetFGMovementComponent()->Velocity;
		const FVector PlayerVelocityOnShootingDir = PlayerVelocity.ProjectOnToNormal(PlayerAimDirection);
		
		Tool->GrappleProjectile = GetWorld()->SpawnActor<AGrappleProjectile>(Tool->GrappleProjectileClass);
		Tool->GrappleProjectile->SetActorLocation(SourceLocation);
		Tool->GrappleProjectile->SetInitialVelocity(PlayerAimDirection * Tool->InitialHookVelocity + PlayerVelocityOnShootingDir);
		Tool->GrappleProjectile->OnProjectileImpactEvent.AddDynamic(Tool, &AGrapplingHookTool::OnGrappleHitSurface);
		Tool->OnRep_GrappleProjectile();
	}
}

void UGrapplingHookRCO::ServerRetractGrapple_Implementation(AGrapplingHookTool* Tool)
{
	if (!Tool)
	{
		return;
	}
	if (Tool->GrappleProjectile)
	{
		Tool->GrappleProjectile->Destroy();
		Tool->GrappleProjectile = nullptr;
	}
	Tool->bGrappleAttached = false;
	Tool->DesiredCableLength = 0;
	Tool->OnRep_DesiredCableLength();
}

void UGrapplingHookRCO::ServerProcessDesiredCableLengthQueries_Implementation(AGrapplingHookTool* Tool, float QueriedChange, float DeltaSeconds)
{
	if (!Tool)
	{
		return;
	}
	
	if (!FMath::IsNearlyZero(QueriedChange))
	{
		float TargetLength = Tool->DesiredCableLength + QueriedChange * DeltaSeconds;
		if (QueriedChange < 0)
		{
			TargetLength = FMath::Min(Tool->DesiredCableLength, // decreasing length must never make it larger than current
				FMath::Max(Tool->GetDistanceToGrappleForcePoint() - 600, // length cannot be too smaller than actual length to avoid slingshot situation  
				TargetLength)); 
		}
		else
		{
			TargetLength = FMath::Min(Tool->GetMaxCableLength(), TargetLength); // length cannot exceed certain maximum value
		}
		
		Tool->DesiredCableLength = TargetLength;
		Tool->DesiredCableLengthControlQuery = 0;
		Tool->OnRep_DesiredCableLength();
	}
}

void UGrapplingHookRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGrapplingHookRCO, bDummy);
}

AGrapplingHookTool::AGrapplingHookTool()
{
}

void AGrapplingHookTool::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocalInstigator())
	{
		ClientTickGrapple(DeltaSeconds);
	}
	if (HasAuthority())
	{
		ServerTickGrapple(DeltaSeconds);
	}
}

void AGrapplingHookTool::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrapplingHookTool, GrappleProjectile);
	DOREPLIFETIME(AGrapplingHookTool, DesiredCableLength);
	DOREPLIFETIME(AGrapplingHookTool, bGrappleAttached);
}

void AGrapplingHookTool::ServerTickGrapple_Implementation(const float DeltaSeconds)
{
	if (!GrappleProjectile)
	{
		return;
	}

	// While grapple is yet flying towards its target
	if (!bGrappleAttached)
	{
		// Return grapple if the shot exceeded maximum allowed cable length
		const float ActualCurrentCableLength = GetDistanceToGrappleForcePoint();
		if (ActualCurrentCableLength > GetMaxCableLength())
		{
			RetractGrapple();
			return;
		}

		// Update cable length until max length is reached (or projectile hits something)
		SetCableLength(FMath::Max(0.1, FMath::Min(GetMaxCableLength(), ActualCurrentCableLength))); 
	}

	if (bGrappleAttached)
	{
		// If player is over tearing distance, retract the grapple
		if (const float DistanceOvershoot = GetDistanceToGrappleForcePoint() - DesiredCableLength;
			DistanceOvershoot >= GetTearingDistance())
		{
			RetractGrapple();
			return;
		}
		
		// Run physics simulation on server (has authority over client simulations, if any are running)
		TickTensionForce(DeltaSeconds, true);

		// Keep visual cable length representing desired length (resulting value is smaller so cable does not appear loose when it should be tense)  
		SetCableLength(FMath::Max(50, DesiredCableLength - 250));
	}
}

void AGrapplingHookTool::SetCableLength_Implementation(const float NewLength)
{
	if (!GrappleProjectile || !GrappleProjectile->CableComponent)
	{
		return;
	}

	// Immediately after a shot, cable behaves erratically on clients.
	// We bandaid this by having a straight cable instead of such broken visuals. 
	if (GetNetMode() == NM_Client && !bGrappleAttached)
	{
		GrappleProjectile->CableComponent->CableLength = 0;
		return;
	}

	GrappleProjectile->CableComponent->CableLength = NewLength;
}

void AGrapplingHookTool::LoadFromItemState_Implementation(const FFGDynamicStruct& itemState)
{
	Super::LoadFromItemState_Implementation(itemState);
	// TODO: maybe save/load projectile and its cable states?	
	UKismetSystemLibrary::PrintString(this, "Grapple load from item state");
}

FFGDynamicStruct AGrapplingHookTool::SaveToItemState_Implementation() const
{ 
	UKismetSystemLibrary::PrintString(this, "Grapple save to item state");	
	// TODO: maybe save/load projectile and its cable states?
	return Super::SaveToItemState_Implementation();

}

bool AGrapplingHookTool::ShouldSave_Implementation() const
{
	// TODO: maybe save/load projectile and its cable states?
	return false;
}

void AGrapplingHookTool::Equip(AFGCharacterPlayer* Character)
{
	Super::Equip(Character);

	AFGPlayerController* Controller = Character->GetFGPlayerController(); 
	SetInputContextRegistered(Controller, true);

	// Create crosshair highlight widget for local tool's owner
	if (Controller && Controller->IsLocalController())
	{
		CrosshairHighlightWidget = CreateWidget(Controller, CrosshairHighlightWidgetClass);
		CrosshairHighlightWidget->AddToPlayerScreen(10);
		CrosshairHighlightWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AGrapplingHookTool::UnEquip()
{
	if (CrosshairHighlightWidget)
	{
		CrosshairHighlightWidget->RemoveFromParent();
		CrosshairHighlightWidget = nullptr;
	}
	
	if (const AFGCharacterPlayer* Character = GetInstigatorCharacter())
	{
		if (AFGPlayerController* Controller = Character->GetFGPlayerController())
		{
			SetInputContextRegistered(Controller, false);
			RetractGrapple();
			ClearEquipmentActionBindings();
		}
	}
	
	Super::UnEquip();
}

void AGrapplingHookTool::AddEquipmentActionBindings()
{
	Super::AddEquipmentActionBindings();

	if (!bInputsBound)
	{
		if (const AFGCharacterPlayer* Player = GetInstigatorCharacter())
		{
			if (AFGPlayerController* Controller = Player->GetFGPlayerController())
			{
				BIND_ACTION(Controller, PrimaryFireAction, Started, "HandleInput_PrimaryFire");
				BIND_ACTION(Controller, RetractCableAction, Triggered, "HandleInput_RetractCable");
				BIND_ACTION(Controller, ExtendCableAction, Triggered, "HandleInput_ExtendCable");
				bInputsBound = true;
			}
		}
	}
}

void AGrapplingHookTool::RetractGrapple()
{
	if (UGrapplingHookRCO* RCO = Cast<AFGPlayerController>(GetInstigatorController())->GetRemoteCallObjectOfClass<UGrapplingHookRCO>())
	{
		RCO->ServerRetractGrapple(this);
		if (!bRetracted)
		{
			bRetracted = true;
			OnGrappleStartedRetracting();
			OnGrappleFinishedRetracting();
		}
	}
}

void AGrapplingHookTool::ClientTickGrapple(const float DeltaSeconds)
{
	if (bGrappleAttached)
	{
		// Apply length control queries to desired cable length 
		if (!FMath::IsNearlyZero(DesiredCableLengthControlQuery))
		{
			if (UGrapplingHookRCO* RCO = Cast<AFGPlayerController>(GetInstigatorController())->GetRemoteCallObjectOfClass<UGrapplingHookRCO>())
			{
				// Length adjustment is processed on server first, and is replicated back to client afterwards
				RCO->ServerProcessDesiredCableLengthQueries(this, DesiredCableLengthControlQuery, DeltaSeconds);
				DesiredCableLengthControlQuery = 0;
			}
		}
	}

	// Update crosshair highlight widget visibility for local player
	if (CrosshairHighlightWidget)
	{
		const AFGCharacterPlayer* Player = GetInstigatorCharacter();
		const UWorld* World = GetWorld();
		bool bShowCrosshairHighlight = false;
		if (Player && World && GrappleProjectileClass && bRetracted)
		{
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(Player);
			if (FHitResult HitResult;
				World->LineTraceSingleByProfile(HitResult,
					GetShootingSourceLocation(),
					GetShootingSourceLocation() + Player->GetBaseAimRotation().Vector() * GetMaxCableLength(),
					"Projectile", QueryParams))
			{
				bShowCrosshairHighlight = true;
			}
		}
		CrosshairHighlightWidget->SetVisibility(bShowCrosshairHighlight ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}		
}

void AGrapplingHookTool::SetInstigatorVelocity_Implementation(const FVector& NewVelocity)
{
	if (const AFGCharacterPlayer* Player = GetInstigatorCharacter())
	{
		if (UFGCharacterMovementComponent* Movement = Player->GetFGMovementComponent())
		{
			Movement->SetGeneralVelocity(NewVelocity);
		}
	}
}

void AGrapplingHookTool::HandleInput_PrimaryFire()
{

	if (UGrapplingHookRCO* RCO = Cast<AFGPlayerController>(GetInstigatorController())->GetRemoteCallObjectOfClass<UGrapplingHookRCO>())
	{
		if (!GrappleProjectile) // shoot projectile if didn't yet
		{
			const AFGCharacterPlayer* Player = GetInstigatorCharacter();
			if (!Player)
			{
				return;
			}

			bRetracted = false;
			const FVector PlayerDirection = Player->GetBaseAimRotation().Vector();
			RCO->ServerShootGrapple(this, GetShootingSourceLocation(), PlayerDirection);
			OnGrappleFired();
		}
		else
		{
			RetractGrapple();
		}
	}
}

void AGrapplingHookTool::HandleInput_RetractCable()
{
	if (!bGrappleAttached)
	{
		return;
	}

	DesiredCableLengthControlQuery -= GetCableLengthControlStep();
}

void AGrapplingHookTool::HandleInput_ExtendCable()
{
	if (!bGrappleAttached)
	{
		return;
	}

	DesiredCableLengthControlQuery += GetCableLengthControlStep();
}

void AGrapplingHookTool::TickTensionForce(const float DeltaSeconds, const bool bPropagateOverNetwork)
{
	const float ActualCurrentCableLength = GetDistanceToGrappleForcePoint();
	if (ActualCurrentCableLength >= DesiredCableLength)
	{
		if (const AFGCharacterPlayer* ToolOwner = GetInstigatorCharacter())
		{
			if (UFGCharacterMovementComponent* Movement = ToolOwner->GetFGMovementComponent())
			{
				// Tension force is player's inverse velocity projected onto grapple direction vector
				const FVector TensionForceDirection = (GetGrappleForcePoint() - ToolOwner->GetActorLocation()).GetSafeNormal();
				const FVector TensionForce = (-Movement->GetVelocity()).ProjectOnTo(TensionForceDirection);

				FVector DesiredVelocity = Movement->Velocity;

				// Apply tension force, if its direction is facing grapple point
				if (FMath::IsNearlyEqual(FVector::DotProduct(TensionForceDirection, TensionForce.GetSafeNormal()), 1.0f, 1e-06))
				{
					// In air, we make the rope feel stretchable by applying the force over time instead of instantly
					// On the ground however it's a different story, because player has friction, tfw much more resistance to external forces - we apply full force immediately.
					const float TimeMultiplier = Movement->MovementMode == MOVE_Walking ? 1 : DeltaSeconds*3;
					DesiredVelocity += TensionForce * TimeMultiplier;  
				}

				// If cable is longer than desired length allows, tension should pull player towards grapple point
				if (ActualCurrentCableLength > DesiredCableLength)
				{
					const float ExcessDistance = ActualCurrentCableLength - DesiredCableLength;
					DesiredVelocity += TensionForceDirection * ExcessDistance * DeltaSeconds * 10;
				}

				// Apply velocity to movement comp
				Movement->Velocity = DesiredVelocity;

				// Help player to automatically take off the ground if floor normal to tension force angle is less than ~43 degrees 
				if (Movement->MovementMode == MOVE_Walking)
				{
					const float Dot = FMath::Clamp(FVector::DotProduct(Movement->CurrentFloor.HitResult.Normal, TensionForceDirection), -1.0f, 1.0f);
					const float FloorNormalToTensionDirAngle = FMath::Acos(Dot); 
					if (FloorNormalToTensionDirAngle < (PI/4 - PI/64))
					{
						Movement->MovementMode = MOVE_Falling;
						Movement->CurrentFloor.Clear();
					}
				}

				if (bPropagateOverNetwork)
				{
					// Manually propagate velocity changes over network to reduce movement lag on clients
					SetInstigatorVelocity(Movement->Velocity);
				}
			}
		}
	}
}

void AGrapplingHookTool::OnGrappleHitSurface(const FHitResult& HitResult)
{
	bGrappleAttached = true;
	DesiredCableLength = GetDistanceToGrappleForcePoint();
	OnRep_GrappleAttached();
	OnRep_DesiredCableLength();
}

float AGrapplingHookTool::GetDistanceToGrappleForcePoint() const
{
	const AFGCharacterPlayer* ToolOwner = GetInstigatorCharacter();
	if (!GrappleProjectile || !ToolOwner)
	{
		return 0;
	}

	// TODO: Implement support for cable turns 
	return FVector::Distance(GetShootingSourceLocation(), GetGrappleForcePoint());
}

FVector AGrapplingHookTool::GetGrappleForcePoint() const
{
	if (!GrappleProjectile)
	{
		return RootComponent->GetComponentLocation();
	}

	// TODO: Implement support for cable turns
	return GrappleProjectile->GetActorLocation();
}

float AGrapplingHookTool::GetDesiredCableLengthQueries() const
{
	return DesiredCableLengthControlQuery;
}

float AGrapplingHookTool::GetMaxCableLength() const
{
	const USessionSettingsManager* SessionSettings = GetWorld()->GetSubsystem<USessionSettingsManager>();
	return SessionSettings->GetFloatOptionValue("AsgGrapplingHook.MaxCableLength");
}

float AGrapplingHookTool::GetTearingDistance() const
{
	const USessionSettingsManager* SessionSettings = GetWorld()->GetSubsystem<USessionSettingsManager>();
	return SessionSettings->GetFloatOptionValue("AsgGrapplingHook.TearingDistance");
}

USceneComponent* AGrapplingHookTool::GetCableAttachComponent_Implementation() const
{
	return RootComponent;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AGrapplingHookTool::SetInputContextRegistered(AFGPlayerController* Controller, const bool bRegistered)
{
	if (!Controller)
	{
		return;
	}
	Controller->SetMappingContextBound(GrappleInputContext, bRegistered);
}

void AGrapplingHookTool::OnRep_GrappleProjectile()
{
	if (GrappleProjectile)
	{
		GrappleProjectile->CableComponent->CableGravityScale = bGrappleAttached ? CableGravityScaleAfterHit : CableGravityScaleBeforeHit;
		GrappleProjectile->CableComponent->SetAttachEndToComponent(GetCableAttachComponent(), CableAttachComponentSocket);
		if (IsLocalInstigator())
		{
			GrappleProjectile->SetFirstPersonCableMaterial();
		}
	}
	else if (!bRetracted)
	{
		bRetracted = true;
		OnGrappleStartedRetracting();
		OnGrappleFinishedRetracting();
	}
}

void AGrapplingHookTool::OnRep_GrappleAttached()
{
	if (bGrappleAttached && GrappleProjectile)
	{
		GrappleProjectile->CableComponent->CableGravityScale = CableGravityScaleAfterHit;
		OnGrappleAttached();
	}
	else if (!bGrappleAttached && !GrappleProjectile && !bRetracted)
	{
		bRetracted = true;
		OnGrappleStartedRetracting();
		OnGrappleFinishedRetracting();
	}
}

void AGrapplingHookTool::OnRep_DesiredCableLength()
{
	OnDesiredCableLengthChanged(DesiredCableLength / GetMaxCableLength());
}
