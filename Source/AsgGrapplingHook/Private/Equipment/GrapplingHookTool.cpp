#include "Equipment/GrapplingHookTool.h"

#include "GrappleLocalConfig.h"
#include "UI/GrappleControlsHintWidget.h"
#include "Misc/DateTime.h"
#include "CableComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Input/FGEnhancedInputComponent.h"
#include "Input/FGInputMappingContext.h"
#include "FGPlayerController.h"
#include "FGHealthComponent.h"
#include "FGSchematicManager.h"
#include "Net/UnrealNetwork.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<float> CVarGrappleClientCompensation(
	TEXT("asg.GrappleClientCompensation"),
	1.5f,
	TEXT("Tension scale on the owning client only (host/server use 1.0). The autonomous proxy retains only ~2/3 of an out-of-band velocity add, so its tension is pre-scaled to match the host. Constant across ping (measured 20-300ms). 1.0 disables it."),
	ECVF_Default);
#include "Kismet/KismetSystemLibrary.h"

#define BIND_ACTION(Controller, Action, TriggerEvent, FuncName) \
	{ \
		FFGRuntimeInputActionDelegate Delegate; \
		Delegate.BindUFunction(this, FuncName); \
		Controller->BindToAction(Action, ETriggerEvent::TriggerEvent, Delegate); \
	}

void UGrapplingHookRCO::ServerReportGrappleFired_Implementation(AGrapplingHookTool* Tool, const FVector& FireLocation, const FVector& FireVelocity)
{
	if (!Tool)
	{
		return;
	}
	Tool->GrappleFireLocation = FireLocation;
	Tool->GrappleFireVelocity = FireVelocity;
	Tool->bGrappleProjectileActive = true;
	Tool->bGrappleAttached = false;
	Tool->DesiredCableLength = 0.f;
	Tool->GrappleForcePointLocation = FVector::ZeroVector;
	Tool->GrappleForcePointRotation = FRotator::ZeroRotator;
	Tool->OnRep_ProjectileActive();
}

void UGrapplingHookRCO::ServerReportGrappleAttached_Implementation(AGrapplingHookTool* Tool, const FVector& AnchorLocation, const FRotator& AnchorRotation, float DesiredLength)
{
	if (!Tool)
	{
		return;
	}
	const float MaxLength = Tool->GetMaxCableLength();
	Tool->ReplicatedReelSpeedMax = Tool->UpgradesSpeed.GetMaximumValue(Tool);
	Tool->GrappleForcePointLocation = AnchorLocation;
	Tool->GrappleForcePointRotation = AnchorRotation;
	Tool->bGrappleAttached = true;
	Tool->bGrappleProjectileActive = true;
	Tool->DesiredCableLength = FMath::Clamp(DesiredLength, 0.f, MaxLength * 1.5f);
	Tool->SetClientAuthoritativeMovement(true);
	Tool->OnRep_ProjectileActive();
	Tool->OnRep_GrappleForcePointLocation();
	Tool->OnRep_GrappleAttached();
	Tool->OnRep_DesiredCableLength();
}

void UGrapplingHookRCO::ServerReportGrappleRetracted_Implementation(AGrapplingHookTool* Tool, const FVector& ReleaseVelocity)
{
	if (!Tool)
	{
		return;
	}
	Tool->ApplyReleaseVelocity(ReleaseVelocity);
	Tool->SetClientAuthoritativeMovement(false);
	Tool->bGrappleProjectileActive = false;
	Tool->bGrappleAttached = false;
	Tool->DesiredCableLength = 0.f;
	Tool->GrappleForcePointLocation = FVector::ZeroVector;
	Tool->OnRep_ProjectileActive();
	Tool->OnRep_GrappleAttached();
	Tool->OnRep_DesiredCableLength();
}

void UGrapplingHookRCO::ServerSetDesiredCableLength_Implementation(AGrapplingHookTool* Tool, float NewLength)
{
	if (!Tool)
	{
		return;
	}
	Tool->DesiredCableLength = FMath::Clamp(NewLength, 0.f, Tool->GetMaxCableLength() * 1.5f);
	Tool->OnRep_DesiredCableLength();
}

void UGrapplingHookRCO::ServerReportGrappleVelocity_Implementation(AGrapplingHookTool* Tool, const FVector& Velocity)
{
	if (!Tool || !Tool->bGrappleAttached)
	{
		return;
	}
	if (const AFGCharacterPlayer* Character = Tool->GetInstigatorCharacter())
	{
		if (UFGCharacterMovementComponent* Movement = Character->GetFGMovementComponent())
		{
			Movement->Velocity = Velocity;
		}
	}
}

void UGrapplingHookRCO::ServerSetGrappleConfig_Implementation(AGrapplingHookTool* Tool, float ReelSpeedRatio, float MaxCableLengthRatio, float EjectionSpeedRatio)
{
	if (!Tool)
	{
		return;
	}
	Tool->UpgradesSpeed.ConfigRatio = FMath::Clamp(ReelSpeedRatio, 0.f, 1.f);
	Tool->UpgradesLength.ConfigRatio = FMath::Clamp(MaxCableLengthRatio, 0.f, 1.f);
	Tool->UpgradesPower.ConfigRatio = FMath::Clamp(EjectionSpeedRatio, 0.f, 1.f);
}

void UGrapplingHookRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGrapplingHookRCO, bDummy);
}

float FGrapplingHookUpgradesChain::GetActiveValue(const UObject* Context) const
{
	return GetActiveValue(Context->GetWorld());
}

float FGrapplingHookUpgradesChain::GetActiveValue(UWorld* Context) const
{
	const float Maximum = GetMaximumValue(Context);
	return FMath::Lerp(BaseValue, Maximum, FMath::Clamp(ConfigRatio, 0.f, 1.f));
}

float FGrapplingHookUpgradesChain::GetMaximumValue(const UObject* Context) const
{
	return GetMaximumValue(Context->GetWorld());
}

float FGrapplingHookUpgradesChain::GetMaximumValue(UWorld* Context) const
{
	float ActiveValue = BaseValue;
	const AFGSchematicManager* SchematicManager = AFGSchematicManager::Get(Context);
	if (!SchematicManager)
	{
		return BaseValue;
	}
	for (const TTuple<TSubclassOf<UFGSchematic>, float>& Upgrade : Upgrades)
	{
		if (Upgrade.Key && SchematicManager->IsSchematicPurchased(Upgrade.Key))
		{
			ActiveValue = FMath::Max(ActiveValue, Upgrade.Value);
		}
	}
	return ActiveValue;
}

AGrapplingHookTool::AGrapplingHookTool()
{
}

void AGrapplingHookTool::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const bool bOwner = IsLocalInstigator();
	const bool bAuthority = HasAuthority();

	UpdateDeathState();
	if (bOwnerDead)
	{
		return;
	}

	if (bOwner)
	{
		ClientTickGrapple(DeltaSeconds);
	}
	if (bAuthority)
	{
		ServerTickGrapple(DeltaSeconds);
	}

	if (!bOwner && GetNetMode() != NM_DedicatedServer)
	{
		if (bGrappleAttached && GrappleProjectile && !GrappleForcePointLocation.IsNearlyZero())
		{
			if (USceneComponent* ProjectileRoot = GrappleProjectile->GetRootComponent(); ProjectileRoot && ProjectileRoot->GetAttachParent())
			{
				ProjectileRoot->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			}
			GrappleProjectile->SetActorLocationAndRotation(GrappleForcePointLocation, GrappleForcePointRotation);
		}
		UpdateCableVisual();
	}
}

void AGrapplingHookTool::UpdateDeathState()
{
	bool bDead = false;
	if (const AFGCharacterPlayer* Character = GetInstigatorCharacter())
	{
		if (const UFGHealthComponent* Health = Character->GetHealthComponent())
		{
			bDead = Health->IsDead();
		}
	}

	if (bDead == bOwnerDead)
	{
		return;
	}
	bOwnerDead = bDead;

	if (bDead)
	{
		if (IsLocalInstigator())
		{
			RetractGrapple();
			if (AFGPlayerController* Controller = Cast<AFGPlayerController>(GetInstigatorController()))
			{
				SetInputContextRegistered(Controller, false);
			}
		}
		else if (HasAuthority())
		{
			SetClientAuthoritativeMovement(false);
		}
	}
	else
	{
		if (IsLocalInstigator())
		{
			if (AFGPlayerController* Controller = Cast<AFGPlayerController>(GetInstigatorController()))
			{
				SetInputContextRegistered(Controller, true);
			}
		}
	}
}

void AGrapplingHookTool::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrapplingHookTool, bGrappleProjectileActive);
	DOREPLIFETIME(AGrapplingHookTool, GrappleFireLocation);
	DOREPLIFETIME(AGrapplingHookTool, GrappleFireVelocity);
	DOREPLIFETIME(AGrapplingHookTool, DesiredCableLength);
	DOREPLIFETIME(AGrapplingHookTool, bGrappleAttached);
	DOREPLIFETIME(AGrapplingHookTool, GrappleForcePointLocation);
	DOREPLIFETIME(AGrapplingHookTool, GrappleForcePointRotation);
	DOREPLIFETIME(AGrapplingHookTool, ReplicatedReelSpeedMax);
}

void AGrapplingHookTool::ServerTickGrapple_Implementation(const float DeltaSeconds)
{
	if (IsLocalInstigator())
	{
		return;
	}
	if (!bGrappleAttached)
	{
		return;
	}
	SetClientAuthoritativeMovement(true);
}

void AGrapplingHookTool::LoadFromItemState_Implementation(const FFGDynamicStruct& itemState)
{
	Super::LoadFromItemState_Implementation(itemState);
}

FFGDynamicStruct AGrapplingHookTool::SaveToItemState_Implementation() const
{
	return Super::SaveToItemState_Implementation();
}

bool AGrapplingHookTool::ShouldSave_Implementation() const
{
	return false;
}

void AGrapplingHookTool::Equip(AFGCharacterPlayer* Character)
{
	Super::Equip(Character);

	if (HasAuthority())
	{
		ReplicatedReelSpeedMax = UpgradesSpeed.GetMaximumValue(this);
	}

	AFGPlayerController* Controller = Character->GetFGPlayerController();
	SetInputContextRegistered(Controller, true);

	if (Controller && Controller->IsLocalController())
	{
		RefreshGrappleConfig();
		ReconcileControlsHintMemory();

		CrosshairHighlightWidget = CreateWidget(Controller, CrosshairHighlightWidgetClass);
		CrosshairHighlightWidget->AddToPlayerScreen(10);
		CrosshairHighlightWidget->SetVisibility(ESlateVisibility::Hidden);

		if (ControlsHintWidgetClass)
		{
			ControlsHintWidget = CreateWidget<UGrappleControlsHintWidget>(Controller, ControlsHintWidgetClass);
			if (ControlsHintWidget)
			{
				ControlsHintWidget->AddToPlayerScreen(9);
				ControlsHintWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void AGrapplingHookTool::UnEquip()
{
	if (CrosshairHighlightWidget)
	{
		CrosshairHighlightWidget->RemoveFromParent();
		CrosshairHighlightWidget = nullptr;
	}
	if (ControlsHintWidget)
	{
		ControlsHintWidget->RemoveFromParent();
		ControlsHintWidget = nullptr;
	}
	bWasAttachedForHint = false;
	bControlsHintVisible = false;

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
	FVector ReleaseVelocity = FVector::ZeroVector;
	if (const AFGCharacterPlayer* Player = GetInstigatorCharacter())
	{
		if (const UFGCharacterMovementComponent* Movement = Player->GetFGMovementComponent())
		{
			ReleaseVelocity = Movement->Velocity;
		}
	}

	const bool bWasActive = bLocalProjectileActive || GrappleProjectile != nullptr;

	DestroyProjectile();
	SetClientAuthoritativeMovement(false);

	bLocalProjectileActive = false;
	bLocalAttached = false;
	bAnchorIsDynamic = false;
	LocalDesiredCableLength = 0.f;
	LocalAnchorLocation = FVector::ZeroVector;
	LocalAnchorRotation = FRotator::ZeroRotator;
	LocalFlightTime = 0.f;
	ReassertOwnerState();

	if (bWasActive)
	{
		OnDesiredCableLengthChanged(0.f);
	}

	if (!HasAuthority())
	{
		if (AFGPlayerController* Controller = Cast<AFGPlayerController>(GetInstigatorController()))
		{
			if (UGrapplingHookRCO* RCO = Controller->GetRemoteCallObjectOfClass<UGrapplingHookRCO>())
			{
				RCO->ServerReportGrappleRetracted(this, ReleaseVelocity);
			}
		}
	}

	if (!bRetracted)
	{
		bRetracted = true;
		OnGrappleStartedRetracting();
		OnGrappleFinishedRetracting();
	}
}

void AGrapplingHookTool::ClientTickGrapple(const float DeltaSeconds)
{
	ReassertOwnerState();

	ConfigSnapCheckAccumulator += DeltaSeconds;
	if (ConfigSnapCheckAccumulator >= 5.0f)
	{
		ConfigSnapCheckAccumulator = 0.f;
		if (ReconcileResearchSnap())
		{
			PushConfigToServer();
		}
	}

	if (bReelInputThisFrame)
	{
		ReelHoldSeconds += DeltaSeconds;
		if (!bReelHoldCounted && ReelHoldSeconds > 0.7f)
		{
			bReelHoldCounted = true;
			RegisterReelSuccess();
		}
	}
	else
	{
		ReelHoldSeconds = 0.f;
		bReelHoldCounted = false;
	}
	bReelInputThisFrame = false;

	if (bExtendInputThisFrame)
	{
		ExtendHoldSeconds += DeltaSeconds;
		if (!bExtendHoldCounted && ExtendHoldSeconds > 0.7f)
		{
			bExtendHoldCounted = true;
			RegisterReelOutSuccess();
		}
	}
	else
	{
		ExtendHoldSeconds = 0.f;
		bExtendHoldCounted = false;
	}
	bExtendInputThisFrame = false;

	if (bGrappleAttached && !bWasAttachedForHint)
	{
		if (ControlsHintWidget && ShouldShowControlsHint())
		{
			bControlsHintVisible = true;
			ControlsHintWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			ControlsHintWidget->PlayShow();
		}
	}
	else if (!bGrappleAttached && bWasAttachedForHint && bControlsHintVisible && ControlsHintWidget)
	{
		bControlsHintVisible = false;
		ControlsHintWidget->PlayHide();
	}
	bWasAttachedForHint = bGrappleAttached;

	if (GrappleProjectile && !bLocalAttached)
	{
		LocalFlightTime += DeltaSeconds;
		const float FlightDistance = GetDistanceToGrappleForcePoint();
		if (FlightDistance > GetMaxCableLength() || LocalFlightTime > 6.f)
		{
			RetractGrapple();
		}
	}

	if (bLocalAttached)
	{
		if (bAnchorIsDynamic && GrappleProjectile)
		{
			LocalAnchorLocation = GrappleProjectile->GetActorLocation();
			GrappleForcePointLocation = LocalAnchorLocation;
		}

		if (GetDistanceToGrappleForcePoint() - LocalDesiredCableLength >= GetTearingDistance())
		{
			RetractGrapple();
		}
		else
		{
			TickTensionForce(DeltaSeconds);

			if (!HasAuthority())
			{
				const AFGCharacterPlayer* VelocityChar = GetInstigatorCharacter();
				const UFGCharacterMovementComponent* VelocityMove = VelocityChar ? VelocityChar->GetFGMovementComponent() : nullptr;
				AFGPlayerController* VelocityController = Cast<AFGPlayerController>(GetInstigatorController());
				UGrapplingHookRCO* VelocityRCO = VelocityController ? VelocityController->GetRemoteCallObjectOfClass<UGrapplingHookRCO>() : nullptr;
				if (VelocityMove && VelocityRCO)
				{
					VelocityRCO->ServerReportGrappleVelocity(this, VelocityMove->Velocity);
				}
			}

			if (!FMath::IsNearlyZero(DesiredCableLengthControlQuery))
			{
				float TargetLength = LocalDesiredCableLength + DesiredCableLengthControlQuery * DeltaSeconds;
				if (DesiredCableLengthControlQuery < 0)
				{
					TargetLength = FMath::Min(LocalDesiredCableLength,
						FMath::Max(GetDistanceToGrappleForcePoint() - 600.f, TargetLength));
				}
				else
				{
					TargetLength = FMath::Min(GetMaxCableLength(), TargetLength);
				}
				LocalDesiredCableLength = TargetLength;
				DesiredCableLength = TargetLength;
				DesiredCableLengthControlQuery = 0;
				OnDesiredCableLengthChanged(GetMaxCableLength() > 0 ? LocalDesiredCableLength / GetMaxCableLength() : 0.f);

				if (!HasAuthority())
				{
					if (AFGPlayerController* Controller = Cast<AFGPlayerController>(GetInstigatorController()))
					{
						if (UGrapplingHookRCO* RCO = Controller->GetRemoteCallObjectOfClass<UGrapplingHookRCO>())
						{
							RCO->ServerSetDesiredCableLength(this, LocalDesiredCableLength);
						}
					}
				}
			}
		}
	}

	UpdateCableVisual();

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

void AGrapplingHookTool::SetClientAuthoritativeMovement(const bool bEnable)
{
	if (!HasAuthority() || IsLocalInstigator())
	{
		return;
	}
	if (const AFGCharacterPlayer* Player = GetInstigatorCharacter())
	{
		if (UFGCharacterMovementComponent* Movement = Player->GetFGMovementComponent())
		{
			Movement->bIgnoreClientMovementErrorChecksAndCorrection = bEnable;
			Movement->bServerAcceptClientAuthoritativePosition = bEnable;
		}
	}
}

void AGrapplingHookTool::ApplyReleaseVelocity(const FVector& ReleaseVelocity)
{
	if (!HasAuthority() || IsLocalInstigator())
	{
		return;
	}
	if (const AFGCharacterPlayer* Player = GetInstigatorCharacter())
	{
		if (UFGCharacterMovementComponent* Movement = Player->GetFGMovementComponent())
		{
			Movement->Velocity = ReleaseVelocity;
		}
	}
}

void AGrapplingHookTool::HandleInput_PrimaryFire()
{
	if (bOwnerDead)
	{
		return;
	}
	if (GrappleProjectile)
	{
		RetractGrapple();
		return;
	}

	const AFGCharacterPlayer* Player = GetInstigatorCharacter();
	if (!Player)
	{
		return;
	}
	const UFGCharacterMovementComponent* Movement = Player->GetFGMovementComponent();

	const FVector FireLocation = GetShootingSourceLocation();
	const FVector AimDirection = Player->GetBaseAimRotation().Vector();

	FVector FireVelocity = AimDirection * GetInitialHookVelocity();
	if (Movement)
	{
		FireVelocity += Movement->Velocity.ProjectOnToNormal(AimDirection);
	}

	bRetracted = false;
	bLocalProjectileActive = true;
	bLocalAttached = false;
	LocalAnchorLocation = FVector::ZeroVector;
	LocalAnchorRotation = FRotator::ZeroRotator;
	LocalDesiredCableLength = 0.f;
	LocalFireLocation = FireLocation;
	LocalFireVelocity = FireVelocity;
	LocalFlightTime = 0.f;
	ReassertOwnerState();

	SpawnSimulatedProjectile(FireLocation, FireVelocity);

	if (!HasAuthority())
	{
		if (AFGPlayerController* Controller = Cast<AFGPlayerController>(GetInstigatorController()))
		{
			if (UGrapplingHookRCO* RCO = Controller->GetRemoteCallObjectOfClass<UGrapplingHookRCO>())
			{
				RCO->ServerReportGrappleFired(this, FireLocation, FireVelocity);
			}
			else
			{
				UE_LOG(LogAsgGrapple, Warning, TEXT("[%s] fire: RCO unavailable, server not informed"), *GrappleNetContext());
			}
		}
	}

	OnGrappleFired();
}

void AGrapplingHookTool::HandleInput_RetractCable()
{
	if (!bGrappleAttached)
	{
		return;
	}

	bReelInputThisFrame = true;
	DesiredCableLengthControlQuery -= GetCableLengthControlStep();
}

void AGrapplingHookTool::HandleInput_ExtendCable()
{
	if (!bGrappleAttached)
	{
		return;
	}

	bExtendInputThisFrame = true;
	DesiredCableLengthControlQuery += GetCableLengthControlStep();
}

void AGrapplingHookTool::TickTensionForce(const float DeltaSeconds)
{
	if (!bGrappleAttached)
	{
		return;
	}
	if (DesiredCableLength <= KINDA_SMALL_NUMBER || GrappleForcePointLocation.IsNearlyZero())
	{
		return;
	}

	const AFGCharacterPlayer* ToolOwner = GetInstigatorCharacter();
	if (!ToolOwner)
	{
		return;
	}
	UFGCharacterMovementComponent* Movement = ToolOwner->GetFGMovementComponent();
	if (!Movement)
	{
		return;
	}

	const float ActualCurrentCableLength = GetDistanceToGrappleForcePoint();
	const FVector TensionForceDirection = (GetGrappleForcePoint() - ToolOwner->GetActorLocation()).GetSafeNormal();

	if (ActualCurrentCableLength >= DesiredCableLength)
	{
		const FVector TensionForce = (-Movement->GetVelocity()).ProjectOnTo(TensionForceDirection);

		FVector TensionDelta = FVector::ZeroVector;

		if (FMath::IsNearlyEqual(FVector::DotProduct(TensionForceDirection, TensionForce.GetSafeNormal()), 1.0f, 1e-06))
		{
			const float TimeMultiplier = Movement->MovementMode == MOVE_Walking ? 1 : DeltaSeconds*3;
			TensionDelta += TensionForce * TimeMultiplier;
		}

		if (ActualCurrentCableLength > DesiredCableLength)
		{
			const float ExcessDistance = FMath::Min(ActualCurrentCableLength - DesiredCableLength, GetTearingDistance());
			TensionDelta += TensionForceDirection * ExcessDistance * DeltaSeconds * 10;
		}

		const float NetworkCompensation = HasAuthority() ? 1.f : CVarGrappleClientCompensation.GetValueOnGameThread();
		Movement->Velocity += TensionDelta * NetworkCompensation;

		if (Movement->MovementMode == MOVE_Walking)
		{
			const float Dot = FMath::Clamp(FVector::DotProduct(Movement->CurrentFloor.HitResult.Normal, TensionForceDirection), -1.0f, 1.0f);
			if (FMath::Acos(Dot) < (PI/4 - PI/64))
			{
				Movement->MovementMode = MOVE_Falling;
				Movement->CurrentFloor.Clear();
			}
		}
	}
}

void AGrapplingHookTool::OnGrappleHitSurface(const FHitResult& HitResult)
{
	if (bLocalAttached)
	{
		return;
	}

	const AFGCharacterPlayer* Player = GetInstigatorCharacter();
	const FVector OwnerLocation = Player ? Player->GetActorLocation() : GetActorLocation();

	UPrimitiveComponent* HitComponent = HitResult.GetComponent();
	bAnchorIsDynamic = HitComponent && HitComponent->Mobility == EComponentMobility::Movable;

	LocalAnchorLocation = HitResult.Location;
	LocalAnchorRotation = GrappleProjectile ? GrappleProjectile->GetActorRotation() : FRotator::ZeroRotator;
	bLocalAttached = true;
	LocalDesiredCableLength = FVector::Distance(OwnerLocation, LocalAnchorLocation);
	LocalFlightTime = 0.f;
	ReassertOwnerState();

	if (GrappleProjectile)
	{
		GrappleProjectile->AttachToAnchor(LocalAnchorLocation, LocalAnchorRotation,
			bAnchorIsDynamic ? HitComponent : nullptr, HitResult.BoneName);
		if (GrappleProjectile->CableComponent)
		{
			GrappleProjectile->CableComponent->CableGravityScale = CableGravityScaleAfterHit;
		}
	}

	OnDesiredCableLengthChanged(GetMaxCableLength() > 0 ? LocalDesiredCableLength / GetMaxCableLength() : 0.f);

	if (!HasAuthority())
	{
		if (AFGPlayerController* Controller = Cast<AFGPlayerController>(GetInstigatorController()))
		{
			if (UGrapplingHookRCO* RCO = Controller->GetRemoteCallObjectOfClass<UGrapplingHookRCO>())
			{
				RCO->ServerReportGrappleAttached(this, LocalAnchorLocation, LocalAnchorRotation, LocalDesiredCableLength);
			}
		}
	}

	OnGrappleAttached();
}

float AGrapplingHookTool::GetDistanceToGrappleForcePoint() const
{
	const AFGCharacterPlayer* ToolOwner = GetInstigatorCharacter();
	if (!ToolOwner)
	{
		return 0;
	}
	if (!bGrappleAttached && !GrappleProjectile)
	{
		return 0;
	}

	// TODO: Implement support for cable turns
	return FVector::Distance(ToolOwner->GetActorLocation(), GetGrappleForcePoint());
}

FVector AGrapplingHookTool::GetGrappleForcePoint() const
{
	if (bGrappleAttached && !GrappleForcePointLocation.IsNearlyZero())
	{
		return GrappleForcePointLocation;
	}

	// TODO: Implement support for cable turns
	if (GrappleProjectile)
	{
		return GrappleProjectile->GetActorLocation();
	}

	if (RootComponent)
	{
		return RootComponent->GetComponentLocation();
	}
	return GetActorLocation();
}

float AGrapplingHookTool::GetDesiredCableLengthQueries() const
{
	return DesiredCableLengthControlQuery;
}

float AGrapplingHookTool::GetCableLengthControlStep() const
{
	const float Maximum = (!HasAuthority() && ReplicatedReelSpeedMax > 0.f)
		? ReplicatedReelSpeedMax
		: UpgradesSpeed.GetMaximumValue(this);
	return FMath::Lerp(UpgradesSpeed.BaseValue, Maximum, FMath::Clamp(UpgradesSpeed.ConfigRatio, 0.f, 1.f));
}

float AGrapplingHookTool::GetMaxCableLength() const
{
	return UpgradesLength.GetActiveValue(this);
}

float AGrapplingHookTool::GetTearingDistance() const
{
	return UpgradesDurability.GetActiveValue(this);
}

float AGrapplingHookTool::GetInitialHookVelocity() const
{
	return UpgradesPower.GetActiveValue(this);
}

void AGrapplingHookTool::SetGrappleConfig(float NewReelSpeedRatio, float NewMaxCableLengthRatio, float NewEjectionSpeedRatio)
{
	UGrappleLocalConfig* Config = UGrappleLocalConfig::Get();

	Config->ReelSpeedRatio = FMath::Clamp(NewReelSpeedRatio, 0.f, 1.f);
	Config->MaxCableLengthRatio = FMath::Clamp(NewMaxCableLengthRatio, 0.f, 1.f);
	Config->EjectionSpeedRatio = FMath::Clamp(NewEjectionSpeedRatio, 0.f, 1.f);

	Config->ReelSpeedMaxSeen = UpgradesSpeed.GetMaximumValue(this);
	Config->MaxCableLengthMaxSeen = UpgradesLength.GetMaximumValue(this);
	Config->EjectionSpeedMaxSeen = UpgradesPower.GetMaximumValue(this);

	Config->Persist();
	ApplyConfigRatios();
	PushConfigToServer();
}

void AGrapplingHookTool::RefreshGrappleConfig()
{
	ReconcileResearchSnap();
	ApplyConfigRatios();
	PushConfigToServer();
}

void AGrapplingHookTool::PushConfigToServer()
{
	if (HasAuthority())
	{
		return;
	}
	if (AFGPlayerController* Controller = Cast<AFGPlayerController>(GetInstigatorController()))
	{
		if (UGrapplingHookRCO* RCO = Controller->GetRemoteCallObjectOfClass<UGrapplingHookRCO>())
		{
			RCO->ServerSetGrappleConfig(this, UpgradesSpeed.ConfigRatio, UpgradesLength.ConfigRatio, UpgradesPower.ConfigRatio);
		}
	}
}

bool AGrapplingHookTool::ReconcileResearchSnap()
{
	UGrappleLocalConfig* Config = UGrappleLocalConfig::Get();
	bool bChanged = false;

	auto Snap = [this, &bChanged](const FGrapplingHookUpgradesChain& Chain, float& Ratio, float& MaxSeen)
	{
		const float CurrentMax = Chain.GetMaximumValue(this);
		if (CurrentMax > MaxSeen + KINDA_SMALL_NUMBER)
		{
			Ratio = 1.f;
			MaxSeen = CurrentMax;
			bChanged = true;
		}
	};

	Snap(UpgradesSpeed, Config->ReelSpeedRatio, Config->ReelSpeedMaxSeen);
	Snap(UpgradesLength, Config->MaxCableLengthRatio, Config->MaxCableLengthMaxSeen);
	Snap(UpgradesPower, Config->EjectionSpeedRatio, Config->EjectionSpeedMaxSeen);

	if (bChanged)
	{
		Config->Persist();
		ApplyConfigRatios();
	}

	return bChanged;
}

void AGrapplingHookTool::ApplyConfigRatios()
{
	const UGrappleLocalConfig* Config = UGrappleLocalConfig::Get();
	UpgradesSpeed.ConfigRatio = FMath::Clamp(Config->ReelSpeedRatio, 0.f, 1.f);
	UpgradesLength.ConfigRatio = FMath::Clamp(Config->MaxCableLengthRatio, 0.f, 1.f);
	UpgradesPower.ConfigRatio = FMath::Clamp(Config->EjectionSpeedRatio, 0.f, 1.f);
}

bool AGrapplingHookTool::ShouldShowControlsHint() const
{
	const UGrappleLocalConfig* Config = UGrappleLocalConfig::Get();
	return Config->ControlsHintReelSuccesses < UGrappleLocalConfig::ControlsHintReelInTarget
		|| Config->ControlsHintReelOutSuccesses < UGrappleLocalConfig::ControlsHintReelOutTarget;
}

void AGrapplingHookTool::RegisterReelSuccess()
{
	UGrappleLocalConfig* Config = UGrappleLocalConfig::Get();
	if (Config->ControlsHintReelSuccesses < UGrappleLocalConfig::ControlsHintReelInTarget)
	{
		Config->ControlsHintReelSuccesses++;
		Config->Persist();
		if (ControlsHintWidget)
		{
			ControlsHintWidget->NotifyTutorialProgress();
		}
	}
}

void AGrapplingHookTool::RegisterReelOutSuccess()
{
	UGrappleLocalConfig* Config = UGrappleLocalConfig::Get();
	if (Config->ControlsHintReelOutSuccesses < UGrappleLocalConfig::ControlsHintReelOutTarget)
	{
		Config->ControlsHintReelOutSuccesses++;
		Config->Persist();
		if (ControlsHintWidget)
		{
			ControlsHintWidget->NotifyTutorialProgress();
		}
	}
}

void AGrapplingHookTool::ReconcileControlsHintMemory()
{
	UGrappleLocalConfig* Config = UGrappleLocalConfig::Get();
	const FDateTime Now = FDateTime::UtcNow();
	if (Config->ControlsHintLastPlayUtcTicks > 0)
	{
		const FDateTime LastPlay = FDateTime(Config->ControlsHintLastPlayUtcTicks);
		if (Now - LastPlay > FTimespan::FromDays(30.0))
		{
			Config->ControlsHintReelSuccesses = 0;
			Config->ControlsHintReelOutSuccesses = 0;
		}
	}
	Config->ControlsHintLastPlayUtcTicks = Now.GetTicks();
	Config->Persist();
}

USceneComponent* AGrapplingHookTool::GetCableAttachComponent_Implementation() const
{
	return RootComponent;
}

void AGrapplingHookTool::SetInputContextRegistered(AFGPlayerController* Controller, const bool bRegistered)
{
	if (!Controller)
	{
		return;
	}
	Controller->SetMappingContextBoundWithHandle(GrappleInputContextHandle, GrappleInputContext, bRegistered);
}

void AGrapplingHookTool::SpawnSimulatedProjectile(const FVector& FireLocation, const FVector& FireVelocity)
{
	UWorld* World = GetWorld();
	if (!World || !GrappleProjectileClass)
	{
		UE_LOG(LogAsgGrapple, Warning, TEXT("[%s] SpawnSimulatedProjectile aborted (world=%d class=%d)"),
			*GrappleNetContext(), World != nullptr, GrappleProjectileClass != nullptr);
		return;
	}

	AFGCharacterPlayer* Player = GetInstigatorCharacter();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = Player;

	GrappleProjectile = World->SpawnActor<AGrappleProjectile>(GrappleProjectileClass, FireLocation, FireVelocity.Rotation(), SpawnParams);
	if (!GrappleProjectile)
	{
		UE_LOG(LogAsgGrapple, Warning, TEXT("[%s] SpawnSimulatedProjectile failed to spawn"), *GrappleNetContext());
		return;
	}

	GrappleProjectile->ConfigureAsSimulated(Player);
	if (Player)
	{
		if (UCapsuleComponent* Capsule = Player->GetCapsuleComponent())
		{
			Capsule->IgnoreActorWhenMoving(GrappleProjectile, true);
		}
	}
	GrappleProjectile->SetInitialVelocity(FireVelocity);
	GrappleProjectile->OnProjectileImpactEvent.AddDynamic(this, &AGrapplingHookTool::OnGrappleHitSurface);
	if (IsLocalInstigator())
	{
		GrappleProjectile->SetFirstPersonCableMaterial();
	}
	if (GrappleProjectile->CableComponent)
	{
		GrappleProjectile->CableComponent->SetAttachEndToComponent(GetCableAttachComponent(), CableAttachComponentSocket);
		GrappleProjectile->CableComponent->CableGravityScale = CableGravityScaleBeforeHit;
	}
}

void AGrapplingHookTool::SpawnCosmeticProjectile(const FVector& FireLocation, const FVector& FireVelocity)
{
	UWorld* World = GetWorld();
	if (!World || !GrappleProjectileClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigatorCharacter();

	GrappleProjectile = World->SpawnActor<AGrappleProjectile>(GrappleProjectileClass, FireLocation, FireVelocity.Rotation(), SpawnParams);
	if (!GrappleProjectile)
	{
		return;
	}

	GrappleProjectile->ConfigureAsCosmetic();
	GrappleProjectile->SetInitialVelocity(FireVelocity);
	if (GrappleProjectile->CableComponent)
	{
		GrappleProjectile->CableComponent->SetAttachEndToComponent(GetCableAttachComponent(), CableAttachComponentSocket);
		GrappleProjectile->CableComponent->CableGravityScale = bGrappleAttached ? CableGravityScaleAfterHit : CableGravityScaleBeforeHit;
	}
}

void AGrapplingHookTool::DestroyProjectile()
{
	if (GrappleProjectile)
	{
		if (AFGCharacterPlayer* Player = GetInstigatorCharacter())
		{
			if (UCapsuleComponent* Capsule = Player->GetCapsuleComponent())
			{
				Capsule->IgnoreActorWhenMoving(GrappleProjectile, false);
			}
		}
		GrappleProjectile->OnProjectileImpactEvent.RemoveDynamic(this, &AGrapplingHookTool::OnGrappleHitSurface);
		GrappleProjectile->Destroy();
		GrappleProjectile = nullptr;
	}
}

void AGrapplingHookTool::ReassertOwnerState()
{
	bGrappleProjectileActive = bLocalProjectileActive;
	bGrappleAttached = bLocalAttached;
	GrappleForcePointLocation = LocalAnchorLocation;
	GrappleForcePointRotation = LocalAnchorRotation;
	DesiredCableLength = LocalDesiredCableLength;
	GrappleFireLocation = LocalFireLocation;
	GrappleFireVelocity = LocalFireVelocity;
}

void AGrapplingHookTool::UpdateCableVisual()
{
	if (!GrappleProjectile || !GrappleProjectile->CableComponent)
	{
		return;
	}

	if (bGrappleAttached)
	{
		GrappleProjectile->CableComponent->CableLength = FMath::Max(50.f, DesiredCableLength - 250.f);
	}
	else
	{
		const USceneComponent* AttachComp = GetCableAttachComponent();
		const float StraightLength = AttachComp
			? FVector::Distance(GrappleProjectile->GetActorLocation(), AttachComp->GetComponentLocation())
			: 0.f;
		GrappleProjectile->CableComponent->CableLength = StraightLength;
	}
}

FString AGrapplingHookTool::GrappleNetContext() const
{
	const TCHAR* Who = IsLocalInstigator() ? TEXT("OWNER") : (HasAuthority() ? TEXT("SERVER") : TEXT("SIM"));
	const TCHAR* Mode;
	switch (GetNetMode())
	{
	case NM_Standalone: Mode = TEXT("SP"); break;
	case NM_DedicatedServer: Mode = TEXT("DS"); break;
	case NM_ListenServer: Mode = TEXT("LS"); break;
	case NM_Client: Mode = TEXT("CL"); break;
	default: Mode = TEXT("?"); break;
	}
	return FString::Printf(TEXT("%s/%s"), Who, Mode);
}

void AGrapplingHookTool::OnRep_ProjectileActive()
{
	if (IsLocalInstigator())
	{
		return;
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (bGrappleProjectileActive)
	{
		if (!GrappleProjectile)
		{
			SpawnCosmeticProjectile(GrappleFireLocation, GrappleFireVelocity);
			bRetracted = false;
		}
	}
	else
	{
		DestroyProjectile();
		if (!bRetracted)
		{
			bRetracted = true;
			OnGrappleStartedRetracting();
			OnGrappleFinishedRetracting();
		}
	}
}

void AGrapplingHookTool::OnRep_GrappleAttached()
{
	if (IsLocalInstigator())
	{
		return;
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (bGrappleAttached && GrappleProjectile)
	{
		if (GrappleProjectile->CableComponent)
		{
			GrappleProjectile->CableComponent->CableGravityScale = CableGravityScaleAfterHit;
		}
		if (!GrappleForcePointLocation.IsNearlyZero())
		{
			GrappleProjectile->AttachToAnchor(GrappleForcePointLocation, GrappleForcePointRotation);
		}
		OnGrappleAttached();
	}
}

void AGrapplingHookTool::OnRep_DesiredCableLength()
{
	if (IsLocalInstigator())
	{
		return;
	}
	OnDesiredCableLengthChanged(GetMaxCableLength() > 0 ? DesiredCableLength / GetMaxCableLength() : 0.f);
}

void AGrapplingHookTool::OnRep_GrappleForcePointLocation()
{
	if (IsLocalInstigator())
	{
		return;
	}
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (bGrappleAttached && GrappleProjectile && !GrappleForcePointLocation.IsNearlyZero())
	{
		GrappleProjectile->AttachToAnchor(GrappleForcePointLocation, GrappleForcePointRotation);
	}
}
