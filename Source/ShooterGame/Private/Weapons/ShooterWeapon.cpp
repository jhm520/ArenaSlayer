// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Particles/ParticleSystemComponent.h"

AShooterWeapon::AShooterWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh1P"));
	Mesh1P->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh1P->bChartDistanceFactor = false;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = Mesh1P;

	Mesh3P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh3P"));
	Mesh3P->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh3P->bChartDistanceFactor = true;
	Mesh3P->bReceivesDecals = false;
	Mesh3P->CastShadow = true;
	Mesh3P->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh3P->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh3P->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	Mesh3P->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh3P->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	Mesh3P->AttachParent = Mesh1P;

	bLoopedMuzzleFX = false;
	bLoopedFireAnim = false;
	bPlayingFireAnim = false;
	bIsEquipped = false;
	bWantsToFire = false;
	bPendingReload = false;
	bPendingEquip = false;
	CurrentState = EWeaponState::Idle;

	CurrentAmmo = 0;
	CurrentAmmoInClip = 0;
	BurstCounter = 0;
	LastFireTime = 0.0f;

	/*John*/
	bBursting = false;
	LastBurstTime = 0.0f;
	bPendingBurst = false;
	BurstStartTime = 0.0f;
	BurstDuration = 0.0f;
	bBurstPausing = false;

	bPendingShot = false;

	//bQuickFiring = false;

	/*End John*/

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bAlwaysRelevant = true;
	bNetUseOwnerRelevancy = true;
}

void AShooterWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (WeaponConfig.InitialClips > 0)
	{
		CurrentAmmoInClip = WeaponConfig.AmmoPerClip;


		//John
		if (WeaponConfig.bNeedsReload)
		{
			CurrentAmmo = WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;
		}
		else
		{
			CurrentAmmo = CurrentAmmoInClip;
			WeaponConfig.MaxAmmo = WeaponConfig.AmmoPerClip;
		}
	}

	//John
	//Had to post initialize this attribute
	//When I set BurstDuration in the object initializer, the program instead 
	//used the default values declared in ShooterWeapon.h

	BurstDuration = WeaponConfig.TimeBetweenShots * WeaponConfig.ShotsPerBurst;

	DetachMeshFromPawn();

	//John
	if (WeaponConfig.bAlwaysEquipped)
	{
		//AttachMeshToPawn();
		bIsEquipped = true;
	}

	
}

void AShooterWeapon::Destroyed()
{
	Super::Destroyed();

	StopSimulatingWeaponFire();
}

//////////////////////////////////////////////////////////////////////////
// Inventory

//John
/** Get this weapon's attach point name*/
FName AShooterWeapon::GetWeaponAttachPoint()
{
	return WeaponAttachPoint;
}

/** Get this weapon's holster attach point*/
FName AShooterWeapon::GetWeaponHolsterPoint()
{
	return WeaponHolsterPoint;
}

//John
/** Equip this weapon without an animation */
void AShooterWeapon::QuickEquip()
{
	//AttachMeshToPawn();
	bPendingEquip = true;
	DetermineWeaponState();
	//AttachMeshToPawn();
	bIsEquipped = true;
	bPendingEquip = false;
	DetermineWeaponState();
}

//John
void AShooterWeapon::QuickUnEquip()
{
	//DetachMeshFromPawn();

	if (!WeaponConfig.bAlwaysEquipped)
	{
		bIsEquipped = false;
	}
	
	StopFire();

	if (bPendingReload)
	{
		StopWeaponAnimation(ReloadAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_StopReload);
		GetWorldTimerManager().ClearTimer(TimerHandle_ReloadWeapon);
	}

	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_OnEquipFinished);
	}

	DetermineWeaponState();
}

void AShooterWeapon::OnEquip()
{
	if (WeaponConfig.EquipAttachTime > 0.0f && MyPawn && !MyPawn->bIsDying)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_AttachMeshToPawn, this, &AShooterWeapon::AttachMeshToPawn,
			WeaponConfig.EquipAttachTime, false);
	}
	else
	{
		AttachMeshToPawn();
	}
	
	bPendingEquip = true;
	DetermineWeaponState();

	float Duration = PlayWeaponAnimation(EquipAnim);
	//if (Duration <= 0.0f)
	//{
	//	// failsafe
	//	Duration = 0.5f;
	//}

	if (WeaponConfig.AltEquipDuration > 0.0f)
	{
		Duration = WeaponConfig.AltEquipDuration;
	}
	
	//John
	//I made it so weapons can have no equip animation so you can use them instantly.
	if (Duration > 0.0f)
	{
		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString::SanitizeFloat(Duration));
		}*/
		EquipStartedTime = GetWorld()->GetTimeSeconds();
		EquipDuration = Duration;

		GetWorldTimerManager().SetTimer(TimerHandle_OnEquipFinished, this, &AShooterWeapon::OnEquipFinished, Duration, false);
	}
	else
	{
		EquipDuration = 0.0;
		OnEquipFinished();
	} 
	/*EquipStartedTime = GetWorld()->GetTimeSeconds();
	EquipDuration = Duration;

	GetWorldTimerManager().SetTimer(TimerHandle_OnEquipFinished, this, &AShooterWeapon::OnEquipFinished, Duration, false);*/

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		PlayWeaponSound(EquipSound);
	}
}

void AShooterWeapon::OnEquipFinished()
{
	
	AttachMeshToPawn();
	

	bIsEquipped = true;
	bPendingEquip = false;

	/*if (MyPawn && MyPawn->IsThrowingGrenade())
	{
		FString YourDebugMessage = FString(TEXT("Finished throwing grenade."));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, YourDebugMessage);
		}
		MyPawn->FinishThrowGrenade();
	}*/ 

	// Determine the state so that the can reload checks will work
	DetermineWeaponState();

	if (MyPawn)
	{
		// try to reload empty clip
		if (MyPawn->IsLocallyControlled() &&
			CurrentAmmoInClip <= 0 &&
			CanReload())
		{
			StartReload();
		}
	}
}

void AShooterWeapon::OnUnEquip (bool bDropped)
{

	if (WeaponConfig.EquipAttachTime > 0.0f && MyPawn && !MyPawn->bIsDying)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_DetachMeshFromPawn, this, &AShooterWeapon::HolsterWeapon,
			WeaponConfig.EquipAttachTime, false);
	}
	else
	{
		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Detached.")));
		}*/
		DetachMeshFromPawn();
	}
	

	bIsEquipped = false;
	StopFire();

	if (bPendingReload)
	{
		StopWeaponAnimation(ReloadAnim);
		bPendingReload = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_StopReload);
		GetWorldTimerManager().ClearTimer(TimerHandle_ReloadWeapon);
	}

	if (bPendingEquip)
	{
		StopWeaponAnimation(EquipAnim);
		bPendingEquip = false;

		GetWorldTimerManager().ClearTimer(TimerHandle_OnEquipFinished);
	}
	
	DetermineWeaponState();
}

void AShooterWeapon::OnEnterInventory(AShooterCharacter* NewOwner)
{
	SetOwningPawn(NewOwner);

	if (WeaponConfig.AttachOnEquip)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Attached")));
		}
		AttachMeshToPawn();
	}
}

void AShooterWeapon::OnLeaveInventory()
{
	if (Role == ROLE_Authority)
	{
		SetOwningPawn(NULL);
	}
	/*if (Role == ROLE_Authority)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Server OnLeave.")));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Client OnLeave.")));
			if (!bIsEquipped)
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Not equipped.")));

			}

			if (!bPendingEquip)
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Not pending equip.")));

			}

		}
	}*/

	//John
	if (Mesh1P->GetAttachParent() || Mesh3P->GetAttachParent())
	{

	}

	//Mesh1P->GetAttachParent() -> This fixed the weapon attachment bug
	if (IsAttachedToPawn() || Mesh1P->GetAttachParent())
	{
		/*FString YourDebugMessage = FString(TEXT("IsAttached."));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, YourDebugMessage);
		}*/
		DetachMeshFromPawn();
		/*if (Role == ROLE_Authority)
		{
		if (GEngine)
		{
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Server Detach.")));
		}
		}
		else
		{
		if (GEngine)
		{
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Client Detach.")));
		}
		}*/


		/*if (Role == ROLE_Authority)
		{
			ClientDetachMeshFromPawn();
		}*/
		OnUnEquip();
	}
}

void AShooterWeapon::AttachMeshToPawn()
{
	if (MyPawn)
	{
		// Remove and hide both first and third person meshes
		DetachMeshFromPawn();

		// For locally controller players we attach both weapons and let the bOnlyOwnerSee, bOwnerNoSee flags deal with visibility.
		//FName AttachPoint = MyPawn->GetWeaponAttachPoint();
		FName AttachPoint = GetWeaponAttachPoint();
		if (MyPawn->IsLocallyControlled() == true)
		{
			USkeletalMeshComponent* PawnMesh1p = MyPawn->GetSpecifcPawnMesh(true);
			USkeletalMeshComponent* PawnMesh3p = MyPawn->GetSpecifcPawnMesh(false);
			Mesh1P->SetHiddenInGame(false);
			Mesh3P->SetHiddenInGame(false);
			Mesh1P->AttachTo(PawnMesh1p, AttachPoint);
			Mesh3P->AttachTo(PawnMesh3p, AttachPoint);
		}
		else
		{
			USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
			USkeletalMeshComponent* UsePawnMesh = MyPawn->GetPawnMesh();
			UseWeaponMesh->AttachTo(UsePawnMesh, AttachPoint);
			UseWeaponMesh->SetHiddenInGame(false);
		}
	}
}

void AShooterWeapon::HolsterWeapon()
{
	
	if (MyPawn)
	{
		//// Remove and hide both first and third person meshes
		DetachMeshFromPawn();
		
		// For locally controller players we attach both weapons and let the bOnlyOwnerSee, bOwnerNoSee flags deal with visibility.
		FName AttachPoint = GetWeaponHolsterPoint();
		if (MyPawn->IsLocallyControlled() == true)
		{
			USkeletalMeshComponent* PawnMesh1p = MyPawn->GetSpecifcPawnMesh(true);
			USkeletalMeshComponent* PawnMesh3p = MyPawn->GetSpecifcPawnMesh(false);
			Mesh1P->SetHiddenInGame(true);
			Mesh3P->SetHiddenInGame(false);
			Mesh1P->AttachTo(PawnMesh1p, AttachPoint);
			Mesh3P->AttachTo(PawnMesh3p, AttachPoint);
		}
		else
		{
			USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
			USkeletalMeshComponent* UsePawnMesh = MyPawn->GetPawnMesh();
			UseWeaponMesh->AttachTo(UsePawnMesh, AttachPoint);
			UseWeaponMesh->SetHiddenInGame(false);
		}
	}
}

void AShooterWeapon::DetachMeshFromPawn()
{

	Mesh1P->DetachFromParent();
	Mesh1P->SetHiddenInGame(true);

	//John
	Mesh3P->DetachFromParent();
	Mesh3P->SetHiddenInGame(true);
}

void AShooterWeapon::ClientDetachMeshFromPawn_Implementation()
{
	DetachMeshFromPawn();
}


//////////////////////////////////////////////////////////////////////////
// Input

//This function is called when the user clicks the mouse button down
void AShooterWeapon::StartFire()
{
	if (WeaponConfig.bBurstWeapon)
	{
		BurstWeapon_StartFire();
		return;
	}

	if (Role < ROLE_Authority)
	{
		ServerStartFire();
	}

	if (!bWantsToFire)
	{
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

//This function is called when the user releases the mouse button.
void AShooterWeapon::StopFire()
{
	if (WeaponConfig.bBurstWeapon)
	{
		return;
	}

	if (Role < ROLE_Authority)
	{
		ServerStopFire();
	}

	if (bWantsToFire)
	{
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

float AShooterWeapon::GetTimeBetweenShots()
{
	return WeaponConfig.TimeBetweenShots;
}

//StartFire handler for Burst Weapons
void AShooterWeapon::BurstWeapon_StartFire()
{
	//Get the time
	float GameTime = GetWorld()->GetTimeSeconds();

	//If start fire was called while the weapon is bursting
	if (bBursting)
	{
		/*If enough time has passed since the last burst, the weapon is 
		no longer bursting
		*/
		if (BurstStartTime > 0.0 && (BurstStartTime + BurstDuration + WeaponConfig.TimeBetweenBursts) <= GameTime)
		{
			//Enough time has passed, clear the timer
			GetWorldTimerManager().ClearTimer(TimerHandle_StartFire);

			//Weapon is no longer pausing
			bBurstPausing = false;

			//Weapon has finished a burst
			bBursting = false;

			//Burst is no longer queued
			bPendingBurst = false;

			//Reset burst start time
			BurstStartTime = 0.0;
		}
		/*if not enough time has passed*/
		else
		{
			/*if a burst hasn't been scheduled yet,
			schedule the next burst for exactly enough time
			for the next burst to start
			*/
			if (!bPendingBurst)
			{
				GetWorldTimerManager().SetTimer(TimerHandle_StartFire, this, &AShooterWeapon::BurstWeapon_StartFire,
					BurstStartTime + BurstDuration + WeaponConfig.TimeBetweenBursts - GameTime, false);
				bPendingBurst = true;
			}
		}
	}

	/*If the weapon is not bursting, we can start a burst*/
	if (!bWantsToFire && !bBursting)
	{
		//Start shooting effects serverwide
		if (Role < ROLE_Authority)
		{
			ServerStartFire();
		}

		/*	Small time offset to prevent the weapon from
		 *	firing more shots than intended.
		 *	This does not change the weapon's fire rate
		 *	burst rate.
		 */
		float TimeOffset = .01;

		/*Schedule for this burst to end after the BurstDuration*/
		GetWorldTimerManager().SetTimer(TimerHandle_StopFire, this, &AShooterWeapon::BurstWeapon_StopFire,
			BurstDuration-TimeOffset, false);

		//Get when this burst started
		BurstStartTime = GameTime;

		/*The weapon is now bursting*/
		bBursting = true;

		/*Do all of the weapon firing stuff*/
		bWantsToFire = true;
		DetermineWeaponState();
	}
}

//John
void AShooterWeapon::SetWeaponProperties(AShooterWeaponPickup* WeaponPickup)
{
	//Setting CurrentAmmoInClip makes the weapon reload after shooting
	CurrentAmmoInClip = WeaponPickup->GetCurrentAmmoInClip();
	CurrentAmmo = WeaponPickup->GetCurrentAmmo();
}

//StopFire handler for burst weapons.
void AShooterWeapon::BurstWeapon_StopFire()
{
	//John
	GetWorldTimerManager().ClearTimer(TimerHandle_StopFire);
	bBurstPausing = true;

	/*Burst weapon cannot stop firing unless it is set to pausing*/
	if (bWantsToFire && bBurstPausing)
	{
		//Stop firing effects serverwide
		if (Role < ROLE_Authority)
		{
			ServerStopFire();
		}

		//Do all of the weapon stop firing stuff
		bWantsToFire = false;
		DetermineWeaponState();
	}
}

bool AShooterWeapon::IsExtraWeapon()
{
	return WeaponConfig.bExtraWeapon;
}

bool AShooterWeapon::IsEquippable()
{
	return WeaponConfig.bEquippable;
}

bool AShooterWeapon::IsDroppable()
{
	return WeaponConfig.bDroppable;
}

void AShooterWeapon::StartReload(bool bFromReplication)
{
	if (!bFromReplication && Role < ROLE_Authority)
	{
		ServerStartReload();
	}

	if (bFromReplication || CanReload())
	{
		bPendingReload = true;
		DetermineWeaponState();

		float AnimDuration = PlayWeaponAnimation(ReloadAnim);
		if (AnimDuration <= 0.0f)
		{
			AnimDuration = WeaponConfig.NoAnimReloadDuration;
		}

		GetWorldTimerManager().SetTimer(TimerHandle_StopReload, this, &AShooterWeapon::StopReload, AnimDuration, false);
		if (Role == ROLE_Authority)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_ReloadWeapon, this, &AShooterWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			PlayWeaponSound(ReloadSound);
		}
	}
}

void AShooterWeapon::StopReload()
{
	if (CurrentState == EWeaponState::Reloading)
	{
		bPendingReload = false;
		DetermineWeaponState();
		StopWeaponAnimation(ReloadAnim);
	}
}

bool AShooterWeapon::ServerStartFire_Validate()
{
	return true;
}

void AShooterWeapon::ServerStartFire_Implementation()
{
	StartFire();
}

bool AShooterWeapon::ServerStopFire_Validate()
{
	return true;
}

void AShooterWeapon::ServerStopFire_Implementation()
{
	StopFire();
}

bool AShooterWeapon::ServerStartReload_Validate()
{
	return true;
}

void AShooterWeapon::ServerStartReload_Implementation()
{
	StartReload();
}

bool AShooterWeapon::ServerStopReload_Validate()
{
	return true;
}

void AShooterWeapon::ServerStopReload_Implementation()
{
	StopReload();
}

void AShooterWeapon::ClientStartReload_Implementation()
{
	StartReload();
}

//////////////////////////////////////////////////////////////////////////
// Control

void AShooterWeapon::SetIsEquipped(bool bEquipped)
{
	bIsEquipped = bEquipped;
}

float AShooterWeapon::GetLungeVelocity()
{
	return WeaponConfig.LungeVelocity;
}

float AShooterWeapon::GetLungeRange()
{
	return WeaponConfig.LungeRange;
}

float AShooterWeapon::GetLungeFinishRange()
{
	return WeaponConfig.LungeFinishRange;
}

FHitResult AShooterWeapon::LungeTrace() const
{

	const FVector AimDir = GetAdjustedAim();
	const FVector StartTrace = GetCameraDamageStartLocation(AimDir);
	const FVector EndTrace = StartTrace + AimDir * WeaponConfig.LungeRange;

	FHitResult Hit = WeaponTrace(StartTrace, EndTrace);

	return Hit;

	/*if (Hit.GetActor() != NULL && Hit.GetActor()->ActorHasTag(FName(TEXT("Damageable"))) && !(Hit.GetActor()->IsRootComponentStatic() || Hit.GetActor()->IsRootComponentStationary()))
	{
		AShooterCharacter* HitChar = Cast<AShooterCharacter>(Hit.GetActor());

		if (HitChar->Health > 0)
		{
			return Hit;
		}
		else
		{
			return Hit;
		}
		
	else
	{
		return Hit;
	}*/
}

/** Get targeting FOV*/
float AShooterWeapon::GetTargetingFOV()
{
	return WeaponConfig.TargetingFOV;
}

float AShooterWeapon::GetTargetingFOV2()
{
	return WeaponConfig.TargetingFOV2;
}

bool AShooterWeapon::CanLunge() const
{
	////If this weapon cannot lunge
	//if (!WeaponConfig.bLunge)
	//{
	//	return false;
	//}

	//const FVector AimDir = GetAdjustedAim();
	//const FVector StartTrace = GetCameraDamageStartLocation(AimDir);
	//const FVector EndTrace = StartTrace + AimDir * WeaponConfig.LungeRange;

	//FHitResult Hit = WeaponTrace(StartTrace, EndTrace);

	//if (Hit.GetActor() != NULL && Hit.GetActor()->ActorHasTag(FName(TEXT("Damageable"))) && !(Hit.GetActor()->IsRootComponentStatic() || Hit.GetActor()->IsRootComponentStationary()))
	//{
	//	AShooterCharacter* HitChar = Cast<AShooterCharacter>(Hit.GetActor()); 

	//	*LungeActor = Hit.GetActor();

	//	return HitChar->Health > 0;
	//}
	//else
	//{
	//	return false;
	//}
	return WeaponConfig.bLunge;
}

bool AShooterWeapon::CanHeadshot()
{
	return WeaponConfig.bHeadshot;
}

bool AShooterWeapon::CanAssassinate()
{
	return WeaponConfig.bAssassinate;
}

bool AShooterWeapon::OffCooldown() const
{
	const float GameTime = GetWorld()->GetTimeSeconds();
	bool bShotOffCooldown = /*LastFireTime > 0.0f && WeaponConfig.TimeBetweenShots > 0.0f && */LastFireTime + WeaponConfig.TimeBetweenShots <= GameTime;
	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString::SanitizeFloat(TraceShape.GetSphereRadius()));
	}*/
	
	if (WeaponConfig.bBurstWeapon)
	{
		bool bBurstOffCooldown = (!bPendingBurst && (BurstStartTime + BurstDuration + WeaponConfig.TimeBetweenBursts) <= GameTime);
		return bShotOffCooldown && bBurstOffCooldown && !bPendingReload;
	}
	else
	{
		return bShotOffCooldown && !bPendingReload;
	}
	
}

bool AShooterWeapon::CanFire() const
{
	bool bCanFire = MyPawn && MyPawn->CanFire();
	bool bStateOKToFire = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return ((bCanFire == true) && (bStateOKToFire == true) && (bPendingReload == false));
}

bool AShooterWeapon::CanReload() const
{
	bool bCanReload = (!MyPawn || MyPawn->CanReload());
	bool bGotAmmo = (CurrentAmmoInClip < WeaponConfig.AmmoPerClip) && (CurrentAmmo - CurrentAmmoInClip > 0 || HasInfiniteClip());
	bool bStateOKToReload = ((CurrentState == EWeaponState::Idle) || (CurrentState == EWeaponState::Firing));
	return ((bCanReload == true) && (bGotAmmo == true) && (bStateOKToReload == true) && WeaponConfig.bNeedsReload);
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AShooterWeapon::GiveAmmo(int AddAmount)
{
	const int32 MissingAmmo = FMath::Max(0, WeaponConfig.MaxAmmo - CurrentAmmo);
	AddAmount = FMath::Min(AddAmount, MissingAmmo);
	CurrentAmmo += AddAmount;

	if (!WeaponConfig.bNeedsReload)
	{
		CurrentAmmoInClip += AddAmount;
	}

	AShooterAIController* BotAI = MyPawn ? Cast<AShooterAIController>(MyPawn->GetController()) : NULL;
	if (BotAI)
	{
		BotAI->CheckAmmo(this);
	}

	// start reload if clip was empty
	if (GetCurrentAmmoInClip() <= 0 &&
		CanReload() &&
		MyPawn->GetWeapon() == this)
	{
		ClientStartReload();
	}
}

void AShooterWeapon::TakeAmmo(int RemoveAmount)
{
	
	RemoveAmount = FMath::Min(RemoveAmount, CurrentAmmo);
	CurrentAmmo -= RemoveAmount;

	if (CurrentAmmo < CurrentAmmoInClip)
	{
		CurrentAmmoInClip = CurrentAmmo;
	}
}

void AShooterWeapon::UseAmmo()
{
	if (!HasInfiniteAmmo())
	{
		CurrentAmmoInClip--;
	}

	if (!HasInfiniteAmmo() && !HasInfiniteClip())
	{
		CurrentAmmo--;
	}

	AShooterAIController* BotAI = MyPawn ? Cast<AShooterAIController>(MyPawn->GetController()) : NULL;
	AShooterPlayerController* PlayerController = MyPawn ? Cast<AShooterPlayerController>(MyPawn->GetController()) : NULL;
	if (BotAI)
	{
		BotAI->CheckAmmo(this);
	}
	else if (PlayerController)
	{
		AShooterPlayerState* PlayerState = Cast<AShooterPlayerState>(PlayerController->PlayerState);
		switch (GetAmmoType())
		{
		case EAmmoType::ERocket:
			PlayerState->AddRocketsFired(1);
			break;
		case EAmmoType::EBullet:
		default:
			PlayerState->AddBulletsFired(1);
			break;
		}
	}
}

void AShooterWeapon::HandleShot()
{

	/*if (Role == ROLE_Authority)
	{*/
		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Server.")));
		}*/
	/*if (Role == ROLE_Authority)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Server handle shot.")));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Client handle shot.")));
		}
	}*/
		
	FireWeapon();

	//UseAmmo();

	////// update firing FX on remote clients if function was called on server
	//BurstCounter++;

	GetWorldTimerManager().ClearTimer(TimerHandle_HandleShot);

	//// local client will notify server
	//if (Role < ROLE_Authority)
	//{
	//	ServerHandleFiring();
	//}

	//// reload after firing last round
	//if (CurrentAmmoInClip <= 0 && CanReload())
	//{
	//	StartReload();
	//}

	//// setup refire timer
	//bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
	//if (bRefiring)
	//{
	//	GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AShooterWeapon::HandleFiring, WeaponConfig.TimeBetweenShots, false);
	//}

		/*if (Role < ROLE_Authority)
		{
			ServerHandleShot();
		}*/
		
	//}
	//else
	//{
	//	if (GEngine)
	//	{
	//		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Client.")));
	//	}
	//}

	
}

void AShooterWeapon::HandleFiring()
{
	if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
	{
		if (GetNetMode() != NM_DedicatedServer)
		{
			/*if (Role == ROLE_Authority)
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Server simulate.")));
				}
			}
			else
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Client simulate.")));
				}
			}*/
			SimulateWeaponFire();
		}

		if (MyPawn && MyPawn->IsLocallyControlled())
		{
			if (WeaponConfig.TimeBeforeShot > 0.0f)
			{
				/*if (Role == ROLE_Authority)
				{
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Server set timer.")));
					}
				}
				else
				{
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Client set timer.")));
					}
				}*/
				GetWorldTimerManager().SetTimer(TimerHandle_HandleShot, this, &AShooterWeapon::HandleShot, WeaponConfig.TimeBeforeShot, false);

				UseAmmo();
				BurstCounter++;
			}
			else
			{
				/*if (Role == ROLE_Authority)
				{
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Server FireWeapon.")));
					}
				}
				else
				{
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Client FireWeapon.")));
					}
				}*/
				//HandleShot();
				FireWeapon();

				UseAmmo();

				// update firing FX on remote clients if function was called on server
				BurstCounter++;
			}
			
		}
		/*else if (MyPawn && !MyPawn->IsLocallyControlled() && WeaponConfig.TimeBeforeShot > 0.0f)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleShot, this, &AShooterWeapon::HandleShot, WeaponConfig.TimeBeforeShot, false);
		}*/
	}
	else if (CanReload())
	{
		StartReload();
	}
	else if (MyPawn && MyPawn->IsLocallyControlled())
	{
		if (GetCurrentAmmo() == 0 && !bRefiring)
		{
			PlayWeaponSound(OutOfAmmoSound);
			AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(MyPawn->Controller);
			AShooterHUD* MyHUD = MyPC ? Cast<AShooterHUD>(MyPC->GetHUD()) : NULL;
			if (MyHUD)
			{
				MyHUD->NotifyOutOfAmmo();
			}
		}

		// stop weapon fire FX, but stay in Firing state
		if (BurstCounter > 0)
		{
			OnBurstFinished();
		}
	}

	if (MyPawn && MyPawn->IsLocallyControlled())
	{
		/*if (Role == ROLE_Authority)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Server notify.")));
			}
		}
		else
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Client notify.")));
			}
		}*/

		// local client will notify server
		if (Role < ROLE_Authority)
		{
			ServerHandleFiring();
		}

		// reload after firing last round
		if (CurrentAmmoInClip <= 0 && CanReload())
		{
			StartReload();
		}

		// setup refire timer
		bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
		if (bRefiring)
		{
			GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AShooterWeapon::HandleFiring, WeaponConfig.TimeBetweenShots, false);
		}
	}

	LastFireTime = GetWorld()->GetTimeSeconds();
}

bool AShooterWeapon::ServerHandleFiring_Validate()
{
	return true;
}

void AShooterWeapon::ServerHandleFiring_Implementation()
{
	const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

	HandleFiring();

	if (bShouldUpdateAmmo || WeaponConfig.TimeBeforeShot > 0.0f)
	{
		// update ammo
		UseAmmo();

		// update firing FX on remote clients
		BurstCounter++;
	}
}

bool AShooterWeapon::ServerHandleShot_Validate()
{
	return true;
}

void AShooterWeapon::ServerHandleShot_Implementation()
{
	HandleShot();
}

void AShooterWeapon::ReloadWeapon()
{
	int32 ClipDelta = FMath::Min(WeaponConfig.AmmoPerClip - CurrentAmmoInClip, CurrentAmmo - CurrentAmmoInClip);

	if (HasInfiniteClip())
	{
		ClipDelta = WeaponConfig.AmmoPerClip - CurrentAmmoInClip;
	}

	if (ClipDelta > 0)
	{
		CurrentAmmoInClip += ClipDelta;
	}

	if (HasInfiniteClip())
	{
		CurrentAmmo = FMath::Max(CurrentAmmoInClip, CurrentAmmo);
	}
}

void AShooterWeapon::SetWeaponState(EWeaponState::Type NewState)
{
	const EWeaponState::Type PrevState = CurrentState;

	if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
	{
		OnBurstFinished();
	}

	CurrentState = NewState;

	if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
	{
		OnBurstStarted();
	}
}

void AShooterWeapon::DetermineWeaponState()
{
	EWeaponState::Type NewState = EWeaponState::Idle;

	if (bIsEquipped)
	{
		if (bPendingReload)
		{
			if (CanReload() == false)
			{
				NewState = CurrentState;
			}
			else
			{
				NewState = EWeaponState::Reloading;
			}
		}
		else if ((bPendingReload == false) && (bWantsToFire == true) && (CanFire() == true))
		{
			NewState = EWeaponState::Firing;
		}
	}
	else if (bPendingEquip)
	{
		NewState = EWeaponState::Equipping;
	}

	SetWeaponState(NewState);
}

void AShooterWeapon::OnBurstStarted()
{
	// start firing, can be delayed to satisfy TimeBetweenShots
	const float GameTime = GetWorld()->GetTimeSeconds();
	if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
		LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
	{
		GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AShooterWeapon::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
	}
	else
	{
		HandleFiring();
	}
}

void AShooterWeapon::OnBurstFinished()
{
	// stop firing FX on remote clients
	BurstCounter = 0;

	// stop firing FX locally, unless it's a dedicated server
	if (GetNetMode() != NM_DedicatedServer)
	{
		StopSimulatingWeaponFire();
	}

	GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
	bRefiring = false;
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers
UAudioComponent* AShooterWeapon::PlayWeaponSound(USoundCue* Sound)
{
	UAudioComponent* AC = NULL;
	if (Sound && MyPawn)
	{
		AC = UGameplayStatics::PlaySoundAttached(Sound, MyPawn->GetRootComponent());
	}

	return AC;
}

float AShooterWeapon::PlayWeaponAnimation(const FWeaponAnim& Animation)
{
	float Duration = 0.0f;
	if (MyPawn)
	{
		UAnimMontage* UseAnim = MyPawn->IsFirstPerson() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			Duration = MyPawn->PlayAnimMontage(UseAnim);
		}
	}

	return Duration;
}

void AShooterWeapon::StopWeaponAnimation(const FWeaponAnim& Animation)
{
	if (MyPawn)
	{
		UAnimMontage* UseAnim = MyPawn->IsFirstPerson() ? Animation.Pawn1P : Animation.Pawn3P;
		if (UseAnim)
		{
			MyPawn->StopAnimMontage(UseAnim);
		}
	}
}

FVector AShooterWeapon::GetCameraAim() const
{
	AShooterPlayerController* const PlayerController = Instigator ? Cast<AShooterPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;

	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		FinalAim = Instigator->GetBaseAimRotation().Vector();
	}

	return FinalAim;
}

FVector AShooterWeapon::GetAdjustedAim() const
{
	AShooterPlayerController* const PlayerController = Instigator ? Cast<AShooterPlayerController>(Instigator->Controller) : NULL;
	FVector FinalAim = FVector::ZeroVector;
	// If we have a player controller use it for the aim
	if (PlayerController)
	{
		FVector CamLoc;
		FRotator CamRot;
		PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
		FinalAim = CamRot.Vector();
	}
	else if (Instigator)
	{
		// Now see if we have an AI controller - we will want to get the aim from there if we do
		AShooterAIController* AIController = MyPawn ? Cast<AShooterAIController>(MyPawn->Controller) : NULL;
		if (AIController != NULL)
		{
			FinalAim = AIController->GetControlRotation().Vector();
		}
		else
		{
			FinalAim = Instigator->GetBaseAimRotation().Vector();
		}
	}

	return FinalAim;
}

FVector AShooterWeapon::GetCameraDamageStartLocation(const FVector& AimDir) const
{
	AShooterPlayerController* PC = MyPawn ? Cast<AShooterPlayerController>(MyPawn->Controller) : NULL;
	AShooterAIController* AIPC = MyPawn ? Cast<AShooterAIController>(MyPawn->Controller) : NULL;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		// use player's camera
		FRotator UnusedRot;
		PC->GetPlayerViewPoint(OutStartTrace, UnusedRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * ((Instigator->GetActorLocation() - OutStartTrace) | AimDir);
	}
	else if (AIPC)
	{
		OutStartTrace = GetMuzzleLocation();
	}

	return OutStartTrace;
}

FVector AShooterWeapon::GetMuzzleLocation() const
{
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketLocation(MuzzleAttachPoint);
}

FVector AShooterWeapon::GetMuzzleDirection() const
{
	USkeletalMeshComponent* UseMesh = GetWeaponMesh();
	return UseMesh->GetSocketRotation(MuzzleAttachPoint).Vector();
}

FHitResult AShooterWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
{
	static FName WeaponFireTag = FName(TEXT("WeaponTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(WeaponFireTag, true, Instigator);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);

	if (WeaponConfig.bSphereTrace)
	{
		
		FQuat TraceRotator(1,0,0,0);
		FCollisionShape TraceShape;
		TraceShape.SetSphere(WeaponConfig.SphereTraceRadius);
		//TraceShape.MakeSphere(WeaponConfig.SphereTraceRadius);
		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString::SanitizeFloat(TraceShape.GetSphereRadius()));
		}*/
		GetWorld()->SweepSingle(Hit, StartTrace, EndTrace, TraceRotator, COLLISION_WEAPON, TraceShape, TraceParams);
	}
	else
	{
		GetWorld()->LineTraceSingle(Hit, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);
	}

	return Hit;
}

bool AShooterWeapon::CanHit() const
{
	const FVector AimDir = GetAdjustedAim();
	const FVector StartTrace = GetCameraDamageStartLocation(AimDir);
	const FVector EndTrace = StartTrace + AimDir * WeaponConfig.ReticuleRange;

	FHitResult Hit = WeaponTrace(StartTrace, EndTrace);

	if (Hit.GetActor() != NULL && Hit.GetActor()->ActorHasTag(FName(TEXT("Damageable"))) && !(Hit.GetActor()->IsRootComponentStatic() || Hit.GetActor()->IsRootComponentStationary()))
	{
		AShooterCharacter* HitChar = Cast<AShooterCharacter>(Hit.GetActor());

		return HitChar->Health > 0;
	}
	else
	{
		return false;
	}
}

void AShooterWeapon::SetOwningPawn(AShooterCharacter* NewOwner)
{
	if (MyPawn != NewOwner)
	{
		Instigator = NewOwner;
		MyPawn = NewOwner;
		// net owner for RPC calls
		SetOwner(NewOwner);
	}
}

//////////////////////////////////////////////////////////////////////////
// Replication & effects

void AShooterWeapon::OnRep_MyPawn()
{
	if (MyPawn)
	{
		OnEnterInventory(MyPawn);
	}
	else
	{
		/*if (Role == ROLE_Authority)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Server OnRep_MyPawn.")));
			}
		}
		else
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Client OnRep_MyPawn.")));
			}
		}*/
		OnLeaveInventory();
	}
}

void AShooterWeapon::OnRep_BurstCounter()
{
	if (BurstCounter > 0)
	{
		SimulateWeaponFire();
	}
	else
	{
		StopSimulatingWeaponFire();
	}
}

void AShooterWeapon::OnRep_Reload()
{
	if (bPendingReload)
	{
		StartReload(true);
	}
	else
	{
		StopReload();
	}
}

void AShooterWeapon::SimulateWeaponFire()
{
	if (Role == ROLE_Authority && CurrentState != EWeaponState::Firing)
	{
		return;
	}

	/*if (Role == ROLE_Authority)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Server Simulate.")));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Client Simulate.")));
		}
	}*/

	if (MuzzleFX)
	{
		USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
		if (!bLoopedMuzzleFX || MuzzlePSC == NULL)
		{
			// Split screen requires we create 2 effects. One that we see and one that the other player sees.
			if ((MyPawn != NULL) && (MyPawn->IsLocallyControlled() == true))
			{
				AController* PlayerCon = MyPawn->GetController();
				if (PlayerCon != NULL)
				{
					Mesh1P->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Mesh1P, MuzzleAttachPoint);
					MuzzlePSC->bOwnerNoSee = false;
					MuzzlePSC->bOnlyOwnerSee = true;

					Mesh3P->GetSocketLocation(MuzzleAttachPoint);
					MuzzlePSCSecondary = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Mesh3P, MuzzleAttachPoint);
					MuzzlePSCSecondary->bOwnerNoSee = true;
					MuzzlePSCSecondary->bOnlyOwnerSee = false;
				}
			}
			else
			{
				MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, UseWeaponMesh, MuzzleAttachPoint);
			}
		}
	}

	if (!bLoopedFireAnim || !bPlayingFireAnim)
	{
		PlayWeaponAnimation(FireAnim);
		bPlayingFireAnim = true;
	}

	if (bLoopedFireSound)
	{
		if (FireAC == NULL)
		{
			FireAC = PlayWeaponSound(FireLoopSound);
		}
	}
	else
	{
		PlayWeaponSound(FireSound);
	}

	AShooterPlayerController* PC = (MyPawn != NULL) ? Cast<AShooterPlayerController>(MyPawn->Controller) : NULL;
	if (PC != NULL && PC->IsLocalController())
	{
		if (FireCameraShake != NULL)
		{
			PC->ClientPlayCameraShake(FireCameraShake, 1);
		}
		if (FireForceFeedback != NULL)
		{
			PC->ClientPlayForceFeedback(FireForceFeedback, false, "Weapon");
		}
	}
}

void AShooterWeapon::StopSimulatingWeaponFire()
{
	if (bLoopedMuzzleFX)
	{
		if (MuzzlePSC != NULL)
		{
			MuzzlePSC->DeactivateSystem();
			MuzzlePSC = NULL;
		}
		if (MuzzlePSCSecondary != NULL)
		{
			MuzzlePSCSecondary->DeactivateSystem();
			MuzzlePSCSecondary = NULL;
		}
	}

	if (bLoopedFireAnim && bPlayingFireAnim)
	{
		StopWeaponAnimation(FireAnim);
		bPlayingFireAnim = false;
	}

	if (FireAC)
	{
		FireAC->FadeOut(0.1f, 0.0f);
		FireAC = NULL;

		PlayWeaponSound(FireFinishSound);
	}
}

void AShooterWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterWeapon, MyPawn);

	DOREPLIFETIME/*_CONDITION*/(AShooterWeapon, CurrentAmmo/*, COND_OwnerOnly*/);
	DOREPLIFETIME/*_CONDITION*/(AShooterWeapon, CurrentAmmoInClip/*, COND_OwnerOnly*/);

	DOREPLIFETIME_CONDITION(AShooterWeapon, BurstCounter, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AShooterWeapon, bPendingReload, COND_SkipOwner);
}

USkeletalMeshComponent* AShooterWeapon::GetWeaponMesh() const
{
	return (MyPawn != NULL && MyPawn->IsFirstPerson()) ? Mesh1P : Mesh3P;
}

class AShooterCharacter* AShooterWeapon::GetPawnOwner() const
{
	return MyPawn;
}

bool AShooterWeapon::IsEquipped() const
{
	return bIsEquipped;
}

bool AShooterWeapon::IsAttachedToPawn() const
{
	return bIsEquipped || bPendingEquip;
}

EWeaponState::Type AShooterWeapon::GetCurrentState() const
{
	return CurrentState;
}

int32 AShooterWeapon::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

int32 AShooterWeapon::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

int32 AShooterWeapon::GetAmmoPerClip() const
{
	return WeaponConfig.AmmoPerClip;
}

int32 AShooterWeapon::GetMaxAmmo() const
{
	return WeaponConfig.MaxAmmo;
}

bool AShooterWeapon::HasInfiniteAmmo() const
{
	const AShooterPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AShooterPlayerController>(MyPawn->Controller) : NULL;
	return WeaponConfig.bInfiniteAmmo || (MyPC && MyPC->HasInfiniteAmmo());
}

bool AShooterWeapon::HasInfiniteClip() const
{
	const AShooterPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AShooterPlayerController>(MyPawn->Controller) : NULL;
	return WeaponConfig.bInfiniteClip || (MyPC && MyPC->HasInfiniteClip());
}

float AShooterWeapon::GetEquipStartedTime() const
{
	return EquipStartedTime;
}

float AShooterWeapon::GetEquipDuration() const
{
	return EquipDuration;
}
