// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"

AShooterCharacter::AShooterCharacter(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UShooterCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("PawnMesh1P"));
	Mesh1P->AttachParent = GetCapsuleComponent();
	Mesh1P->bOnlyOwnerSee = true;
	Mesh1P->bOwnerNoSee = false;
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	Mesh1P->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	Mesh1P->bChartDistanceFactor = false;
	Mesh1P->SetCollisionObjectType(ECC_Pawn);
	Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);

	GetMesh()->bOnlyOwnerSee = false;
	GetMesh()->bOwnerNoSee = true;
	GetMesh()->bReceivesDecals = false;
	GetMesh()->SetCollisionObjectType(ECC_Pawn);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);
	
	

	TargetingSpeedModifier = 0.5f;
	bIsTargeting = false;
	bCanRun = false;
	RunningSpeedModifier = 1.5f;
	bWantsToRun = false;
	bWantsToFire = false;
	LowHealthPercentage = 0.3f;

	//bQuickFiring = false;

	/*John*/
	/*Health Regen attributes*/
	bHealthRegen = false;
	RegenPerSecond = 0.0;
	RegenTickRate = 0.0;
	TimeBeforeRegen = 0.0;
	LastDamageTime = 0.0;
	LastRegenTime = 0.0;
	StartedRegen = false;

	bLunging = false;

	LungeFinished = false;

	/*PrevWeapon = NULL;
	QuickFiringWeapon = NULL;*/

	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	LungeActor = NULL;

	Shields = NULL;
}

void AShooterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Role == ROLE_Authority)
	{
		Health = GetMaxHealth();
		SpawnDefaultInventory();
	}

	Shields = FindComponentByClass<UStaticMeshComponent>();
	SetShields(false);

	

	//SpawnDefaultInventory();

	// set initial mesh visibility (3rd person view)
	UpdatePawnMeshes();
	
	// create material instance for setting team colors (3rd person view)
	for (int32 iMat = 0; iMat < GetMesh()->GetNumMaterials(); iMat++)
	{
		MeshMIDs.Add(GetMesh()->CreateAndSetMaterialInstanceDynamic(iMat));
	} 

	// play respawn effects
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (RespawnFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(this, RespawnFX, GetActorLocation(), GetActorRotation());
		}

		if (RespawnSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, RespawnSound, GetActorLocation());
		}
	}
}

void AShooterCharacter::Destroyed()
{
	Super::Destroyed();
	DestroyInventory();
}

void AShooterCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// switch mesh to 1st person view
	UpdatePawnMeshes();

	// reattach weapon if needed
	SetCurrentWeapon(CurrentWeapon);

	// set team colors for 1st person view
	UMaterialInstanceDynamic* Mesh1PMID = Mesh1P->CreateAndSetMaterialInstanceDynamic(0);
	UpdateTeamColors(Mesh1PMID);
}

void AShooterCharacter::PossessedBy(class AController* InController)
{
	Super::PossessedBy(InController);

	// [server] as soon as PlayerState is assigned, set team colors of this pawn for local player
	UpdateTeamColorsAllMIDs();
}

void AShooterCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// [client] as soon as PlayerState is assigned, set team colors of this pawn for local player
	if (PlayerState != NULL)
	{
		UpdateTeamColorsAllMIDs();
	}
}

FRotator AShooterCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

//John
/**Get camera aim*/
FVector AShooterCharacter::GetCameraAim() const
{
	//Maybe you can cast an AActor as an AShooterWeapon
	AShooterPlayerController* const PlayerController = this ? Cast<AShooterPlayerController>(this->Controller) : NULL;
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

	/*if (Role == ROLE_Authority)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Yellow, FinalAim.ToString());
		}
	}*/
	
	return FinalAim;
}

//John
void AShooterCharacter::StartLunge(AShooterWeapon* LungingWeapon)
{
	if (Role == ROLE_Authority)
	{
		LungeWeapon = LungingWeapon;

		LungeHit = LungeWeapon->LungeTrace();

		if (LungeHit.GetActor() != NULL && LungeHit.GetActor()->ActorHasTag(FName(TEXT("Damageable"))) && !(LungeHit.GetActor()->IsRootComponentStatic() || LungeHit.GetActor()->IsRootComponentStationary()))
		{
			AShooterCharacter* HitChar = Cast<AShooterCharacter>(LungeHit.GetActor());

			//*LungeActor = LungeHit.GetActor();

			if (HitChar->Health > 0)
			{
				Lunge();
				return;
			}
		}
		/*else
		{
			return false;
		}
		
		Lunge();*/
		if (LungeWeapon == CurrentWeapon)
		{
			ClientFireWeapon();
		}
		else
		{
			ClientFireExtraWeapon(LungeWeapon);
		}
	}
	else
	{
		ServerStartLunge(LungingWeapon);
	}
}

void AShooterCharacter::ServerStartLunge_Implementation(AShooterWeapon* LungingWeapon)
{
	
	StartLunge(LungingWeapon);
}

bool AShooterCharacter::ServerStartLunge_Validate(AShooterWeapon* LungingWeapon)
{
	return true;
}

//John
void AShooterCharacter::Lunge()
{

	LungeStartLocation = GetActorLocation();
	if (LungeActor)
	{
		LungeFinishLocation = LungeActor->GetActorLocation();
	}

	bLunging = true;
	
	UCharacterMovementComponent* PawnMove = GetCharacterMovement();

	PrevWalkDecel = PawnMove->BrakingDecelerationWalking;
	PrevGroundFriction = PawnMove->GroundFriction;
	PrevGravScale = PawnMove->GravityScale;
	PrevMaxWalkSpeed = PawnMove->MaxWalkSpeed;

	PawnMove->BrakingDecelerationWalking = 0;
	PawnMove->GroundFriction = 0;
	PawnMove->GravityScale = 0;

	PawnMove->MaxWalkSpeed = LungeWeapon->GetLungeVelocity();
	PawnMove->Velocity = GetCameraAim()*LungeWeapon->GetLungeVelocity();
}

void AShooterCharacter::FinishLunge()
{
	if (Role == ROLE_Authority)
	{
		UCharacterMovementComponent* PawnMove = GetCharacterMovement();
		PawnMove->BrakingDecelerationWalking = PrevWalkDecel;
		PawnMove->GroundFriction = PrevGroundFriction;
		PawnMove->GravityScale = PrevGravScale;
		PawnMove->Velocity = FVector::ZeroVector;
		PawnMove->MaxWalkSpeed = PrevMaxWalkSpeed;

		if (LungeWeapon == CurrentWeapon)
		{
			ClientFireWeapon();
		}
		else
		{
			ClientFireExtraWeapon(LungeWeapon);
		}

		PrevWalkDecel = 0;
		PrevGroundFriction = 0;
		PrevGravScale = 0;
		PrevMaxWalkSpeed = 0;

		GetWorldTimerManager().ClearTimer(TimerHandle_Lunge);
		LungeWeapon = NULL;
		bLunging = false;
	}
	else
	{
		ServerFinishLunge();
	}
	
}

void AShooterCharacter::ServerFinishLunge_Implementation()
{
	FinishLunge();
}

bool AShooterCharacter::ServerFinishLunge_Validate()
{
	return true;
}

void AShooterCharacter::CheckLungeFinished()
{

	if (Role == ROLE_Authority)
	{
		if (bLunging)
		{
			//if (GEngine)
			//{
			//	GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Lunging")));
			//}
			FVector CurrentLocation = GetActorLocation();
			if (FVector::Dist(CurrentLocation, LungeStartLocation) > LungeWeapon->GetLungeRange())
			{
				FinishLunge();
			}
			/*else if (GetVelocity().Size() < 100.0f)
			{
				FinishLunge();
			}*/
			else if (FVector::Dist(CurrentLocation, LungeHit.Location) <= LungeWeapon->GetLungeFinishRange())
			{
				FinishLunge();
			}
			else if (LungeFinished)
			{
				LungeFinished = false;
				FinishLunge();
			}
		}
	}
	else
	{
		ServerCheckLungeFinished();
	}
}

void AShooterCharacter::ServerCheckLungeFinished_Implementation()
{
	CheckLungeFinished();
}

bool AShooterCharacter::ServerCheckLungeFinished_Validate()
{
	return true;
}

void AShooterCharacter::ReceiveHit(UPrimitiveComponent * MyComp,
	AActor * Other,
	UPrimitiveComponent * OtherComp,
	bool bSelfMoved,
	FVector HitLocation,
	FVector HitNormal,
	FVector NormalImpulse,
	const FHitResult & Hit)
{
	Super::ReceiveHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	if (bLunging)
	{
		LungeFinished = true;
		CheckLungeFinished();
	}

}

//John
/**Get camera start location*/
FVector AShooterCharacter::GetCameraStartLocation(const FVector& AimDir) const
{
	AShooterPlayerController* PC = this ? Cast<AShooterPlayerController>(this->Controller) : NULL;
	//AShooterAIController* AIPC = Instigator ? Cast<AShooterAIController>(Instigator->Controller) : NULL;
	FVector OutStartTrace = FVector::ZeroVector;

	if (PC)
	{
		// use player's camera
		FRotator UnusedRot;
		PC->GetPlayerViewPoint(OutStartTrace, UnusedRot);

		// Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
		OutStartTrace = OutStartTrace + AimDir * ((Instigator->GetActorLocation() - OutStartTrace) | AimDir);
	}
	/*else if (AIPC)
	{
		OutStartTrace = GetMuzzleLocation();
	}*/

	return OutStartTrace;
}

//John
/** find what object the player has interacted with */
FHitResult AShooterCharacter::InteractTrace(const FVector& StartTrace, const FVector& EndTrace) const
{
	static FName InteractTag = FName(TEXT("InteractTrace"));

	// Perform trace to retrieve hit info
	FCollisionQueryParams TraceParams(InteractTag, true, this);
	TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = true;

	FHitResult Hit(ForceInit);
	GetWorld()->LineTraceSingle(Hit, StartTrace, EndTrace, COLLISION_WEAPON, TraceParams);

	return Hit;
}

bool AShooterCharacter::IsEnemyFor(AController* TestPC) const
{
	if (TestPC == Controller || TestPC == NULL)
	{
		return false;
	}

	AShooterPlayerState* TestPlayerState = Cast<AShooterPlayerState>(TestPC->PlayerState);
	AShooterPlayerState* MyPlayerState = Cast<AShooterPlayerState>(PlayerState);

	bool bIsEnemy = true;
	if (GetWorld()->GameState && GetWorld()->GameState->GameModeClass)
	{
		const AShooterGameMode* DefGame = GetWorld()->GameState->GameModeClass->GetDefaultObject<AShooterGameMode>();
		if (DefGame && MyPlayerState && TestPlayerState)
		{
			bIsEnemy = DefGame->CanDealDamage(TestPlayerState, MyPlayerState);
		}
	}

	return bIsEnemy;
}


//////////////////////////////////////////////////////////////////////////
// Meshes

void AShooterCharacter::UpdatePawnMeshes()
{
	bool const bFirstPerson = IsFirstPerson();

	Mesh1P->MeshComponentUpdateFlag = !bFirstPerson ? EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered : EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	Mesh1P->SetOwnerNoSee(!bFirstPerson);

	GetMesh()->MeshComponentUpdateFlag = bFirstPerson ? EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered : EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetOwnerNoSee(bFirstPerson);
}

void AShooterCharacter::UpdateTeamColors(UMaterialInstanceDynamic* UseMID)
{
	if (UseMID)
	{
		AShooterPlayerState* MyPlayerState = Cast<AShooterPlayerState>(PlayerState);
		if (MyPlayerState != NULL)
		{
			float MaterialParam = (float)MyPlayerState->GetTeamNum();
			UseMID->SetScalarParameterValue(TEXT("Team Color Index"), MaterialParam);
		}
	}
}

void AShooterCharacter::OnCameraUpdate(const FVector& CameraLocation, const FRotator& CameraRotation)
{
	USkeletalMeshComponent* DefMesh1P = Cast<USkeletalMeshComponent>(GetClass()->GetDefaultSubobjectByName(TEXT("PawnMesh1P")));
	const FMatrix DefMeshLS = FRotationTranslationMatrix(DefMesh1P->RelativeRotation, DefMesh1P->RelativeLocation);
	const FMatrix LocalToWorld = ActorToWorld().ToMatrixWithScale();

	// Mesh rotating code expect uniform scale in LocalToWorld matrix

	const FRotator RotCameraPitch(CameraRotation.Pitch, 0.0f, 0.0f);
	const FRotator RotCameraYaw(0.0f, CameraRotation.Yaw, 0.0f);

	const FMatrix LeveledCameraLS = FRotationTranslationMatrix(RotCameraYaw, CameraLocation) * LocalToWorld.Inverse();
	const FMatrix PitchedCameraLS = FRotationMatrix(RotCameraPitch) * LeveledCameraLS;
	const FMatrix MeshRelativeToCamera = DefMeshLS * LeveledCameraLS.Inverse();
	const FMatrix PitchedMesh = MeshRelativeToCamera * PitchedCameraLS;

	Mesh1P->SetRelativeLocationAndRotation(PitchedMesh.GetOrigin(), PitchedMesh.Rotator());
}


//////////////////////////////////////////////////////////////////////////
// Damage & death


void AShooterCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	Die(Health, FDamageEvent(dmgType.GetClass()), NULL, NULL);
}

void AShooterCharacter::Suicide()
{
	KilledBy(this);
}

void AShooterCharacter::KilledBy(APawn* EventInstigator)
{
	if (Role == ROLE_Authority && !bIsDying)
	{
		AController* Killer = NULL;
		if (EventInstigator != NULL)
		{
			Killer = EventInstigator->Controller;
			LastHitBy = NULL;
		}

		Die(Health, FDamageEvent(UDamageType::StaticClass()), Killer, NULL);
	}
}

/*John*/
bool AShooterCharacter::Regenerating() const
{
	return true;
}

void AShooterCharacter::Regenerate()
{
	
	if (Health >= GetMaxHealth())
	{
		Health = GetMaxHealth();
		GetWorldTimerManager().ClearTimer(TimerHandle_Regenerate);
		return;
	}

	if (!StartedRegen)
	{
		//Play the sound
		if (RegenSound)
		{
			UGameplayStatics::PlaySoundAttached(RegenSound, GetRootComponent());
		}
		StartedRegen = true;
	}

	Health += RegenPerSecond * RegenTickRate;
	GetWorldTimerManager().SetTimer(TimerHandle_Regenerate, this, &AShooterCharacter::Regenerate,
		RegenTickRate, false);
}

bool AShooterCharacter::ShieldsDown()
{
	return (Health <= (GetMaxHealth()*LowHealthPercentage));
}

//John
/**Check whether or not this actor can headshot.*/
bool AShooterCharacter::CanHeadshot(class AActor* DamageCauser)
{
	//Get headshot tag from Actor
	char* HeadshotStr = "Headshot";

	//BOOM HEADSHOT
	FName Headshot = FName(HeadshotStr);

	return DamageCauser->ActorHasTag(Headshot);
}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->HasGodMode())
	{
		return 0.f;
	}

	if (Health <= 0.f)
	{
		return 0.f;
	}

	// Modify based on game rules.
	AShooterGameMode* const Game = GetWorld()->GetAuthGameMode<AShooterGameMode>();
	Damage = Game ? Game->ModifyDamage(Damage, this, DamageEvent, EventInstigator, DamageCauser) : 0.f;


	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.f)
	{
		Health -= ActualDamage;
		
		FHitResult ThisHit;
		FVector ThisHitVector;

		//Get Hit Info from the damage event
		DamageEvent.GetBestHitInfo(this, EventInstigator, ThisHit, ThisHitVector);

		AShooterWeapon* DamageCauserWeapon = Cast<AShooterWeapon>(DamageCauser);

		if (DamageCauserWeapon)
		{
			//Check for headshot
			if (ShieldsDown() && DamageCauserWeapon->CanHeadshot())//CanHeadshot(DamageCauser))
			{
				//If the pawn was hit in the head
				if (ThisHit.BoneName.ToString() == "b_head")
				{
					//Dead
					Health = 0;
				}
			}

			//Check for assassination
			if (DamageCauserWeapon->CanAssassinate() && FVector::Coincident(ThisHitVector, GetCameraAim(), .5) && (ThisHit.BoneName.ToString() == "b_neck" || ThisHit.BoneName.ToString() == "b_spine1" || ThisHit.BoneName.ToString() == "b_spine"))
			{
				Health = 0;
			}
		}

		

		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, ThisHit.BoneName.ToString());
		}*/
		

		//If the pawn was hit in the back
		//Assassination
		/*if (ThisHit.BoneName.ToString() == "b_neck" || ThisHit.BoneName.ToString() == "b_spine1" || ThisHit.BoneName.ToString() == "b_spine")
		{
			Health = 0;
		}*/

		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Yellow, ThisHitVector.ToString());
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 1, FColor::Yellow, GetCameraAim().ToString());
		}*/

		

		if (Health <= 0)
		{
			Die(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
			if (bHealthRegen)
			{
				GetWorldTimerManager().ClearTimer(TimerHandle_Regenerate);
			}
		}
		else
		{
			PlayHit(ActualDamage, DamageEvent, EventInstigator ? EventInstigator->GetPawn() : NULL, DamageCauser);

			/*John*/
			/*Health regen implemented here*/
			/*If the player has health regen and takes damage, the regenerate
			function is scheduled to call after TimeBeforeRegen,
			Regenerate() calls itself until the player is full, or takes damage again
			*/
			if (bHealthRegen)
			{
				StartedRegen = false;
				/*if (Shields)
				{
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, Shields->ComponentTags[0].ToString());
					}
					SetShields(true);
				}*/
				
				if (GetWorldTimerManager().IsTimerActive(TimerHandle_Regenerate))
				{
					GetWorldTimerManager().ClearTimer(TimerHandle_Regenerate);
				}
				GetWorldTimerManager().SetTimer(TimerHandle_Regenerate, this, &AShooterCharacter::Regenerate,
					TimeBeforeRegen, false);
			}
		}

		MakeNoise(1.0f, EventInstigator ? EventInstigator->GetPawn() : this);
	}

	return ActualDamage;
}

void AShooterCharacter::SetShields(bool bShieldsUp)
{
	//ClientSetShields(bShieldsUp);

	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("ServerSetShields")));
	}*/

	if (bShieldsUp)
	{
		Shields->SetVisibility(true);
		Shields->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
		Shields->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
	}
	else
	{
		Shields->SetVisibility(false);
		Shields->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Ignore);
		Shields->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);
	}
}

//void AShooterCharacter::ClientSetShields_Implementation(bool bShieldsUp)
//{
//	/*if (GEngine)
//	{
//		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("ClientSetShields")));
//	}*/
//	Shields->SetVisibility(true);
//	Shields->SetCollisionResponseToChannel(COLLISION_PROJECTILE, ECR_Block);
//	Shields->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Block);
//}


bool AShooterCharacter::CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const
{
	if ( bIsDying										// already dying
		|| IsPendingKill()								// already destroyed
		|| Role != ROLE_Authority						// not authority
		|| GetWorld()->GetAuthGameMode() == NULL
		|| GetWorld()->GetAuthGameMode()->GetMatchState() == MatchState::LeavingMap)	// level transition occurring
	{
		return false;
	}

	return true;
}


bool AShooterCharacter::Die(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (!CanDie(KillingDamage, DamageEvent, Killer, DamageCauser))
	{
		return false;
	}

	Health = FMath::Min(0.0f, Health);

	// if this is an environmental death then refer to the previous killer so that they receive credit (knocked into lava pits, etc)
	UDamageType const* const DamageType = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
	Killer = GetDamageInstigator(Killer, *DamageType);

	AController* const KilledPlayer = (Controller != NULL) ? Controller : Cast<AController>(GetOwner());
	GetWorld()->GetAuthGameMode<AShooterGameMode>()->Killed(Killer, KilledPlayer, this, DamageType);

	NetUpdateFrequency = GetDefault<AShooterCharacter>()->NetUpdateFrequency;
	GetCharacterMovement()->ForceReplicationUpdate();

	OnDeath(KillingDamage, DamageEvent, Killer ? Killer->GetPawn() : NULL, DamageCauser);
	return true;
}


void AShooterCharacter::OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (bIsDying)
	{
		return;
	}

	bReplicateMovement = false;
	bTearOff = true;
	bIsDying = true;

	if (Role == ROLE_Authority)
	{
		ReplicateHit(KillingDamage, DamageEvent, PawnInstigator, DamageCauser, true);	

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->KilledForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->KilledForceFeedback, false, "Damage");
			}
		}
	}

	// cannot use IsLocallyControlled here, because even local client's controller may be NULL here
	if (GetNetMode() != NM_DedicatedServer && DeathSound && Mesh1P && Mesh1P->IsVisible())
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	// remove all weapons
	DestroyInventory();
	
	// switch back to 3rd person view
	UpdatePawnMeshes();

	DetachFromControllerPendingDestroy();
	StopAllAnimMontages();

	if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
	{
		LowHealthWarningPlayer->Stop();
	}

	if (RunLoopAC)
	{
		RunLoopAC->Stop();
	}

	// disable collisions on capsule
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);

	if (GetMesh())
	{
		static FName CollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetCollisionProfileName(CollisionProfileName);
	}
	SetActorEnableCollision(true);

	// Death anim
	float DeathAnimDuration = PlayAnimMontage(DeathAnim);

	// Ragdoll
	if (DeathAnimDuration > 0.f)
	{
		// Use a local timer handle as we don't need to store it for later but we don't need to look for something to clear
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, this, &AShooterCharacter::SetRagdollPhysics, FMath::Min(0.1f, DeathAnimDuration), false);
	}
	else
	{
		SetRagdollPhysics();
	}
}

void AShooterCharacter::PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser)
{
	if (Role == ROLE_Authority)
	{
		ReplicateHit(DamageTaken, DamageEvent, PawnInstigator, DamageCauser, false);

		// play the force feedback effect on the client player controller
		APlayerController* PC = Cast<APlayerController>(Controller);
		if (PC && DamageEvent.DamageTypeClass)
		{
			UShooterDamageType *DamageType = Cast<UShooterDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject());
			if (DamageType && DamageType->HitForceFeedback)
			{
				PC->ClientPlayForceFeedback(DamageType->HitForceFeedback, false, "Damage");
			}
		}
	}

	if (DamageTaken > 0.f)
	{
		ApplyDamageMomentum(DamageTaken, DamageEvent, PawnInstigator, DamageCauser);
	}
	
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	AShooterHUD* MyHUD = MyPC ? Cast<AShooterHUD>(MyPC->GetHUD()) : NULL;
	if (MyHUD)
	{
		MyHUD->NotifyHit(DamageTaken, DamageEvent, PawnInstigator);
	}

	if (PawnInstigator && PawnInstigator != this && PawnInstigator->IsLocallyControlled())
	{
		AShooterPlayerController* InstigatorPC = Cast<AShooterPlayerController>(PawnInstigator->Controller);
		AShooterHUD* InstigatorHUD = InstigatorPC ? Cast<AShooterHUD>(InstigatorPC->GetHUD()) : NULL;
		if (InstigatorHUD)
		{
			InstigatorHUD->NotifyEnemyHit();
		}
	}
}


void AShooterCharacter::SetRagdollPhysics()
{
	bool bInRagdoll = false;

	if (IsPendingKill())
	{
		bInRagdoll = false;
	}
	else if (!GetMesh() || !GetMesh()->GetPhysicsAsset())
	{
		bInRagdoll = false;
	}
	else
	{
		// initialize physics/etc
		GetMesh()->SetAllBodiesSimulatePhysics(true);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		GetMesh()->bBlendPhysics = true;

		bInRagdoll = true;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	if (!bInRagdoll)
	{
		// hide and set short lifespan
		TurnOff();
		SetActorHiddenInGame(true);
		SetLifeSpan( 1.0f );
	}
	else
	{
		SetLifeSpan( 10.0f );
	}
}



void AShooterCharacter::ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser, bool bKilled)
{
	const float TimeoutTime = GetWorld()->GetTimeSeconds() + 0.5f;

	FDamageEvent const& LastDamageEvent = LastTakeHitInfo.GetDamageEvent();
	if ((PawnInstigator == LastTakeHitInfo.PawnInstigator.Get()) && (LastDamageEvent.DamageTypeClass == LastTakeHitInfo.DamageTypeClass) && (LastTakeHitTimeTimeout == TimeoutTime))
	{
		// same frame damage
		if (bKilled && LastTakeHitInfo.bKilled)
		{
			// Redundant death take hit, just ignore it
			return;
		}

		// otherwise, accumulate damage done this frame
		Damage += LastTakeHitInfo.ActualDamage;
	}

	LastTakeHitInfo.ActualDamage = Damage;
	LastTakeHitInfo.PawnInstigator = Cast<AShooterCharacter>(PawnInstigator);
	LastTakeHitInfo.DamageCauser = DamageCauser;
	LastTakeHitInfo.SetDamageEvent(DamageEvent);		
	LastTakeHitInfo.bKilled = bKilled;
	LastTakeHitInfo.EnsureReplication();

	LastTakeHitTimeTimeout = TimeoutTime;
}

void AShooterCharacter::OnRep_LastTakeHitInfo()
{
	if (LastTakeHitInfo.bKilled)
	{
		OnDeath(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
	else
	{
		PlayHit(LastTakeHitInfo.ActualDamage, LastTakeHitInfo.GetDamageEvent(), LastTakeHitInfo.PawnInstigator.Get(), LastTakeHitInfo.DamageCauser.Get());
	}
}

//Pawn::PlayDying sets this lifespan, but when that function is called on client, dead pawn's role is still SimulatedProxy despite bTearOff being true. 
void AShooterCharacter::TornOff()
{
	SetLifeSpan(25.f);
}


//////////////////////////////////////////////////////////////////////////
// Inventory

//John
//Character drops their equipped weapon and tosses it in front of him
void AShooterCharacter::DropWeapon()
{
	if (Role == ROLE_Authority)
	{
		if (CurrentWeapon)
		{
			AShooterWeapon* RemovedWeapon = CurrentWeapon;
			OnNextWeapon();
			//This works at removing the weapon

			if (Inventory.Num() > 1)
			{
				FActorSpawnParameters SpawnInfo;
				SpawnInfo.bNoCollisionFail = true;
				FVector NewLocation = GetActorLocation() + FVector(0.f, 0.f, 100.f);
				FRotator NewRotator = GetActorRotation() + FRotator(0.f, -135.f, 0.f);

				//Spawn new weapon pickup to match removed weapon
				AShooterWeaponPickup* NewPickup = GetWorld()->SpawnActor<AShooterWeaponPickup>(RemovedWeapon->WeaponPickup, NewLocation, NewRotator, SpawnInfo);

				NewPickup->SetWeaponPickup(RemovedWeapon);

				//Get the player's current velocity.
				FVector NewPickupVelocity = GetVelocity();

				//Add "throw weapon" velocity
				//This could be a uproperty
				NewPickupVelocity += GetCameraAim() * DropWeaponVelocity;

				NewPickup->WeaponMesh->SetAllPhysicsLinearVelocity(NewPickupVelocity, false);

				RemoveWeapon(RemovedWeapon);
			}
		}
	}
	else
	{
		ServerDropWeapon();
	}
}

//John
/** Check if the player can interact with an object
 *	You should figure out a way around this
 *	redundance.
 *	Perhaps a method called CanInteractWith(), that returns
 *	the object that you can interact with. Or as an out parameter.
 */
bool AShooterCharacter::CanInteract(AActor** OutObject) const
{

	/**Get what the player is pointing at*/
	const FVector AimDir = GetCameraAim();
	const FVector StartTrace = GetCameraStartLocation(AimDir);
	const FVector EndTrace = StartTrace + AimDir * InteractRange;
	const FHitResult Interaction = InteractTrace(StartTrace, EndTrace);

	/*The object the player is pointing at*/
	AActor* InteractableObject = Interaction.GetActor();

	*OutObject = InteractableObject;

	if (InteractableObject && InteractableObject->ActorHasTag(FName(TEXT("Interact"))))
	{
		return true;
	}

	/*Get all objects the player is standing on.*/
	TArray<class AActor*> ProximityInteractableObjects;

	GetOverlappingActors(ProximityInteractableObjects);

	if (ProximityInteractableObjects.Num() > 0)
	{
		InteractableObject = ProximityInteractableObjects[0];
	}

	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString::FromInt(ProximityInteractableObjects.Num()));
	}*/

	*OutObject = InteractableObject;

	if (InteractableObject && InteractableObject->ActorHasTag(FName(TEXT("Interact"))))
	{
		return true;
	}

	return false;
}

//John
/**Character interacts with an interactable object*/
void AShooterCharacter::Interact()
{
	if (Role == ROLE_Authority)
	{
		/**Get what the player is pointing at*/
		const FVector AimDir = GetCameraAim();
		const FVector StartTrace = GetCameraStartLocation(AimDir);
		const FVector EndTrace = StartTrace + AimDir * InteractRange;
		const FHitResult Interaction = InteractTrace(StartTrace, EndTrace);

		/*The object the player is pointing at*/
		AActor* InteractedObject = Interaction.GetActor();

		/**If the object the player is pointing at is interactable*/
		if (InteractedObject && InteractedObject->ActorHasTag(FName(TEXT("Interact"))))
		{
			FPointDamageEvent PointDmg;
			PointDmg.DamageTypeClass = NULL;
			PointDmg.HitInfo = Interaction;
			PointDmg.ShotDirection = FVector(0.0);
			PointDmg.Damage = 0.0;

			APlayerController* PC = Cast<APlayerController>(Controller);

			/**Call TakeDamage with 0 damage,
			which will serve as an interaction
			Consider trying to find a different function
			to perform interaction

			in this case, TakeDamage will place
			the weapon in the user's inventory*/
			InteractedObject->TakeDamage(0.0, PointDmg, PC, this);
			return;
		}

		/**	If the function hasn't returned yet, that means the player wasn't pointing at an interactable object.
		 *	Now we will detect proximity interactable objects
		 */

		TArray<class AActor*> ProximityInteractableObjects;

		/*Get all objects the player is standing on.*/
		GetOverlappingActors(ProximityInteractableObjects);

		if (ProximityInteractableObjects.Num() > 0)
		{
			InteractedObject = ProximityInteractableObjects[0];
		}

		/*if (GEngine)
		{
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString::FromInt(ProximityInteractableObjects.Num()));
		}*/

		if (InteractedObject && InteractedObject->ActorHasTag(FName(TEXT("Interact"))))
		{
			FPointDamageEvent PointDmg;
			PointDmg.DamageTypeClass = NULL;
			PointDmg.HitInfo = Interaction;
			PointDmg.ShotDirection = FVector(0.0);
			PointDmg.Damage = 0.0;

			APlayerController* PC = Cast<APlayerController>(Controller);

			/**Call TakeDamage with 0 damage,
			which will serve as an interaction
			Consider trying to find a different function
			to perform interaction

			in this case, TakeDamage will place
			the weapon in the user's inventory*/
			InteractedObject->TakeDamage(0.0, PointDmg, PC, this);
		}
	}
	else
	{
		ServerInteract();
	}
}

void AShooterCharacter::ClientFireExtraWeapon_Implementation(AShooterWeapon* ExtraWeapon)
{
	ExtraWeapon->QuickEquip();
	ExtraWeapon->StartFire();
	ExtraWeapon->StopFire();
	ExtraWeapon->QuickUnEquip();
}

void AShooterCharacter::FireExtraWeapon(AShooterWeapon* ExtraWeapon)
{
	ExtraWeapon->QuickEquip();
	ExtraWeapon->StartFire();
	ExtraWeapon->StopFire();
	ExtraWeapon->QuickUnEquip();
}

void AShooterCharacter::ServerFireExtraWeapon_Implementation(AShooterWeapon* ExtraWeapon)
{
	FireExtraWeapon(ExtraWeapon);
}

bool AShooterCharacter::ServerFireExtraWeapon_Validate(AShooterWeapon* ExtraWeapon)
{
	return true;
}

void AShooterCharacter::ClientFireWeapon_Implementation()
{
	StartWeaponFire();
	StopWeaponFire();
}

int32 AShooterCharacter::GetMaxInventory()
{
	return MaxInventory;
}

bool AShooterCharacter::InventoryFull()
{

	int32 NumWeapons = 0;

	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (!Inventory[i]->IsExtraWeapon())
		{
			NumWeapons++;
		}
	}

	return NumWeapons >= GetMaxInventory();
}

void AShooterCharacter::SpawnDefaultInventory()
{
	if (Role < ROLE_Authority)
	{
		return;
	}

	int32 NumWeaponClasses = DefaultInventoryClasses.Num();
	
	for (int32 i = 0; i < NumWeaponClasses; i++)
	{
		if (DefaultInventoryClasses[i])
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.bNoCollisionFail = true;
			AShooterWeapon* NewWeapon = GetWorld()->SpawnActor<AShooterWeapon>(DefaultInventoryClasses[i], SpawnInfo);
			AddWeapon(NewWeapon);

		}

	}

	// equip first weapon in inventory
	if (Inventory.Num() > 0)
	{
		EquipWeapon(Inventory[0]);
	}
}

void AShooterCharacter::ServerSpawnDefaultInventory_Implementation()
{
	SpawnDefaultInventory();
}

bool AShooterCharacter::ServerSpawnDefaultInventory_Validate()
{
	return true;
}

void AShooterCharacter::DestroyInventory()
{
	if (Role < ROLE_Authority)
	{
		return;
	}

	// remove all weapons from inventory and destroy them
	for (int32 i = Inventory.Num() - 1; i >= 0; i--)
	{
		AShooterWeapon* Weapon = Inventory[i];
		if (Weapon)
		{
			RemoveWeapon(Weapon);
			Weapon->Destroy();
		}
	}
}

void AShooterCharacter::AddWeapon(AShooterWeapon* Weapon)
{
	if (Weapon && Role == ROLE_Authority)
	{
		Weapon->OnEnterInventory(this);
		Inventory.AddUnique(Weapon);
	}
}

void AShooterCharacter::RemoveWeapon(AShooterWeapon* Weapon)
{
	if (Weapon && Role == ROLE_Authority)
	{
		Weapon->OnLeaveInventory();
		Inventory.RemoveSingle(Weapon);
	}
}

AShooterWeapon* AShooterCharacter::FindWeapon(TSubclassOf<AShooterWeapon> WeaponClass)
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] && Inventory[i]->IsA(WeaponClass))
		{
			return Inventory[i];
		}
	}

	return NULL;
}

void AShooterCharacter::EquipWeapon(AShooterWeapon* Weapon)
{
	if (Weapon)
	{
		if (Role == ROLE_Authority)
		{
			SetCurrentWeapon(Weapon);
		}
		else
		{
			ServerEquipWeapon(Weapon);
		}
	}
}

bool AShooterCharacter::ServerEquipWeapon_Validate(AShooterWeapon* Weapon)
{
	return true;
}

void AShooterCharacter::ServerEquipWeapon_Implementation(AShooterWeapon* Weapon)
{
	EquipWeapon(Weapon);
}

//John
/** Drop weapon server methods
 */
bool AShooterCharacter::ServerDropWeapon_Validate()
{
	return true;
}

void AShooterCharacter::ServerDropWeapon_Implementation()
{
	DropWeapon();
}

/** Interact server methods
 */

bool AShooterCharacter::ServerInteract_Validate()
{
	return true;
}

void AShooterCharacter::ServerInteract_Implementation()
{
	Interact();
}

void AShooterCharacter::OnRep_CurrentWeapon(AShooterWeapon* LastWeapon)
{
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void AShooterCharacter::SetCurrentWeapon(class AShooterWeapon* NewWeapon, class AShooterWeapon* LastWeapon)
{
	AShooterWeapon* LocalLastWeapon = NULL;
	
	if (LastWeapon != NULL)
	{
		LocalLastWeapon = LastWeapon;
	}
	else if (NewWeapon != CurrentWeapon)
	{
		LocalLastWeapon = CurrentWeapon;
	}

	// unequip previous
	if (LocalLastWeapon)
	{
		LocalLastWeapon->OnUnEquip();
	}

	CurrentWeapon = NewWeapon;

	// equip new one
	if (NewWeapon)
	{
		NewWeapon->SetOwningPawn(this);	// Make sure weapon's MyPawn is pointing back to us. During replication, we can't guarantee APawn::CurrentWeapon will rep after AWeapon::MyPawn!
		NewWeapon->OnEquip();
	}
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AShooterCharacter::StartWeaponFire()
{
	if (!bWantsToFire)
	{
		bWantsToFire = true;
		if (CurrentWeapon)
		{
			//if (GEngine)
			//{
			//	GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, CurrentWeapon->Tags[0].ToString());
			//}
			if (CurrentWeapon->CanLunge() && InventoryOffCooldown(false) && !bLunging)
			{
				StartLunge(CurrentWeapon);
			}
			else if (InventoryOffCooldown(true))
			{
				CurrentWeapon->StartFire();
			}
		}
	}
}

void AShooterCharacter::StopWeaponFire()
{
	if (bWantsToFire)
	{
		bWantsToFire = false;
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
		}
	}
}

bool AShooterCharacter::CanFire() const
{
	return IsAlive();
}

bool AShooterCharacter::CanReload() const
{
	return true;
}

bool AShooterCharacter::InventoryOffCooldown(bool bExceptCurrent) const
{
	int InvCount = GetInventoryCount();

	for (int i = 0; i < InvCount; i++)
	{
		if (Inventory[i])
		{
			if (bExceptCurrent && Inventory[i] == CurrentWeapon)
			{
				continue;
			}
			else if (!Inventory[i]->OffCooldown())
			{
				return false;
			}
		}
	}
	return true;
}

void AShooterCharacter::SetTargeting(bool bNewTargeting)
{
	bIsTargeting = bNewTargeting;

	//John
	if (TargetingSound)
	{
		UGameplayStatics::PlaySoundAttached(TargetingSound, GetRootComponent());
	}

	if (Role < ROLE_Authority)
	{
		ServerSetTargeting(bNewTargeting);
	}
}

bool AShooterCharacter::ServerSetTargeting_Validate(bool bNewTargeting)
{
	return true;
}

void AShooterCharacter::ServerSetTargeting_Implementation(bool bNewTargeting)
{
	SetTargeting(bNewTargeting);
}

//////////////////////////////////////////////////////////////////////////
// Movement

void AShooterCharacter::SetRunning(bool bNewRunning, bool bToggle)
{
	bWantsToRun = bNewRunning;
	bWantsToRunToggled = bNewRunning && bToggle;

	if (Role < ROLE_Authority)
	{
		ServerSetRunning(bNewRunning, bToggle);
	}

	UpdateRunSounds(bNewRunning);
	
}

bool AShooterCharacter::ServerSetRunning_Validate(bool bNewRunning, bool bToggle)
{
	return true;
}

void AShooterCharacter::ServerSetRunning_Implementation(bool bNewRunning, bool bToggle)
{
	SetRunning(bNewRunning, bToggle);
}

void AShooterCharacter::UpdateRunSounds(bool bNewRunning)
{
	if (bNewRunning)
	{
		if (!RunLoopAC && RunLoopSound)
		{
			RunLoopAC = UGameplayStatics::PlaySoundAttached(RunLoopSound, GetRootComponent());
			if (RunLoopAC)
			{
				RunLoopAC->bAutoDestroy = false;
			}
			
		}
		else if (RunLoopAC)
		{
			RunLoopAC->Play();
		}
	}
	else
	{
		if (RunLoopAC)
		{
			RunLoopAC->Stop();
		}

		if (RunStopSound)
		{
			UGameplayStatics::PlaySoundAttached(RunStopSound, GetRootComponent());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Animations

float AShooterCharacter::PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate, FName StartSectionName) 
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance)
	{
		return UseMesh->AnimScriptInstance->Montage_Play(AnimMontage, InPlayRate);
	}

	return 0.0f;
}

void AShooterCharacter::StopAnimMontage(class UAnimMontage* AnimMontage)
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (AnimMontage && UseMesh && UseMesh->AnimScriptInstance &&
		UseMesh->AnimScriptInstance->Montage_IsPlaying(AnimMontage))
	{
		UseMesh->AnimScriptInstance->Montage_Stop(AnimMontage->BlendOutTime);
	}
}

void AShooterCharacter::StopAllAnimMontages()
{
	USkeletalMeshComponent* UseMesh = GetPawnMesh();
	if (UseMesh && UseMesh->AnimScriptInstance)
	{
		UseMesh->AnimScriptInstance->Montage_Stop(0.0f);
	}
}


//////////////////////////////////////////////////////////////////////////
// Input

void AShooterCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	check(InputComponent);
	InputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	InputComponent->BindAxis("MoveUp", this, &AShooterCharacter::MoveUp);
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);

	InputComponent->BindAction("Fire", IE_Pressed, this, &AShooterCharacter::OnStartFire);
	InputComponent->BindAction("Fire", IE_Released, this, &AShooterCharacter::OnStopFire);

	InputComponent->BindAction("Targeting", IE_Pressed, this, &AShooterCharacter::OnStartTargeting);
	InputComponent->BindAction("Targeting", IE_Released, this, &AShooterCharacter::OnStopTargeting);

	InputComponent->BindAction("NextWeapon", IE_Pressed, this, &AShooterCharacter::OnSwitchWeapon);
	InputComponent->BindAction("PrevWeapon", IE_Pressed, this, &AShooterCharacter::OnSwitchWeapon);

	//John
	InputComponent->BindAction("DropWeapon", IE_Pressed, this, &AShooterCharacter::OnDropWeapon);

	InputComponent->BindAction("Interact", IE_Pressed, this, &AShooterCharacter::OnInteract);

	InputComponent->BindAction("ThrowFragGrenade", IE_Pressed, this, &AShooterCharacter::OnThrowFragGrenade);
	InputComponent->BindAction("ThrowStickyGrenade", IE_Pressed, this, &AShooterCharacter::OnThrowStickyGrenade);

	InputComponent->BindAction("Melee", IE_Pressed, this, &AShooterCharacter::OnMelee);

	InputComponent->BindAction("Reload", IE_Pressed, this, &AShooterCharacter::OnReload);

	InputComponent->BindAction("Jump", IE_Pressed, this, &AShooterCharacter::OnStartJump);
	InputComponent->BindAction("Jump", IE_Released, this, &AShooterCharacter::OnStopJump);

	InputComponent->BindAction("Run", IE_Pressed, this, &AShooterCharacter::OnStartRunning);
	InputComponent->BindAction("RunToggle", IE_Pressed, this, &AShooterCharacter::OnStartRunningToggle);
	InputComponent->BindAction("Run", IE_Released, this, &AShooterCharacter::OnStopRunning);
}


void AShooterCharacter::MoveForward(float Val)
{
	if (Controller && Val != 0.f)
	{
		if (!bLunging)
		{
			// Limit pitch when walking or falling
			const bool bLimitRotation = (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling());
			const FRotator Rotation = bLimitRotation ? GetActorRotation() : Controller->GetControlRotation();
			const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::X);
			AddMovementInput(Direction, Val);
		}
	}
}

void AShooterCharacter::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		if (!bLunging)
		{
			const FRotator Rotation = GetActorRotation();
			const FVector Direction = FRotationMatrix(Rotation).GetScaledAxis(EAxis::Y);
			AddMovementInput(Direction, Val);
		}
	}
}

void AShooterCharacter::MoveUp(float Val)
{
	if (Val != 0.f)
	{
		if (!bLunging)
		{
			// Not when walking or falling.
			if (GetCharacterMovement()->IsMovingOnGround() || GetCharacterMovement()->IsFalling())
			{
				return;
			}

			AddMovementInput(FVector::UpVector, Val);
		}
	}
}

void AShooterCharacter::TurnAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Val * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::OnStartFire()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		StartWeaponFire();
	}
}

void AShooterCharacter::OnStopFire()
{
	StopWeaponFire();
}

void AShooterCharacter::OnStartTargeting()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (IsRunning())
		{
			SetRunning(false, false);
		}
		SetTargeting(true);
	}
}

void AShooterCharacter::OnStopTargeting()
{
	SetTargeting(false);
}

void AShooterCharacter::OnNextWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2)// && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AShooterWeapon* NextWeapon = Inventory[(CurrentWeaponIdx + 1) % Inventory.Num()];
			EquipWeapon(NextWeapon);
		}
	}
}

void AShooterCharacter::OnPrevWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2)// && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			const int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AShooterWeapon* PrevWeapon = Inventory[(CurrentWeaponIdx - 1 + Inventory.Num()) % Inventory.Num()];
			EquipWeapon(PrevWeapon);
		}
	}
}

void AShooterCharacter::OnEquipWeapon(AShooterWeapon* Weapon)
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		/*if (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping)
		{*/
			EquipWeapon(Weapon);
			/*if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("Equipped")));
			}*/
		/*}*/
	}
}

void AShooterCharacter::OnSwitchWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (Inventory.Num() >= 2)// && (CurrentWeapon == NULL || CurrentWeapon->GetCurrentState() != EWeaponState::Equipping))
		{
			int32 CurrentWeaponIdx = Inventory.IndexOfByKey(CurrentWeapon);
			AShooterWeapon* NextWeapon = Inventory[(CurrentWeaponIdx = CurrentWeaponIdx + 1) % Inventory.Num()];

			while (!NextWeapon->IsEquippable())
			{
				NextWeapon = Inventory[(CurrentWeaponIdx = CurrentWeaponIdx + 1) % Inventory.Num()];
			}

			EquipWeapon(NextWeapon);
		}
	}
}

//John
void AShooterCharacter::OnDropWeapon()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (CurrentWeapon && CurrentWeapon->WeaponPickup)//&& CurrentWeapon->GetCurrentState() != EWeaponState::Equipping)
		{
			DropWeapon();
		}
	}
}

//John
void AShooterCharacter::OnInteract()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		Interact();
	}
}

void AShooterCharacter::OnThrowFragGrenade()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		AShooterWeapon* FragGrenadeWeapon = NULL;
		if (FragGrenade)
		{
			FragGrenadeWeapon = FindWeapon(FragGrenade);
		}

		if (FragGrenadeWeapon && FragGrenadeWeapon->GetCurrentAmmo() > 0)
		{
			//StartQuickFire(FragGrenadeWeapon);
			if (InventoryOffCooldown())
			{
				/*FragGrenadeWeapon->QuickEquip();
				FragGrenadeWeapon->StartFire();
				FragGrenadeWeapon->StopFire();
				FragGrenadeWeapon->QuickUnEquip();*/

				FireExtraWeapon(FragGrenadeWeapon);
			}
		}
	}
}

void AShooterCharacter::OnThrowStickyGrenade()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		AShooterWeapon* StickyGrenadeWeapon = NULL;
		if (StickyGrenade)
		{
			StickyGrenadeWeapon = FindWeapon(StickyGrenade);
		}

		if (StickyGrenadeWeapon && StickyGrenadeWeapon->GetCurrentAmmo() > 0)
		{
			//StartQuickFire(StickyGrenadeWeapon);
			if (InventoryOffCooldown())
			{
				FireExtraWeapon(StickyGrenadeWeapon);
			}
		}
	}
}

void AShooterCharacter::OnMelee()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		AShooterWeapon* MeleeWeapon = NULL;
		if (Melee)
		{
			MeleeWeapon = FindWeapon(Melee);
		}

		if (MeleeWeapon && MeleeWeapon->GetCurrentAmmo() > 0 && !bLunging)
		{
			//if (MeleeWeapon->CanLunge())
			//{
			//	//bQuickFireLunge = true;
			//	//OnEquipWeapon(MeleeWeapon);
			//	StartLunge();
			//}
			//else
			//{
			//	StartQuickFire(MeleeWeapon);
			//}
			if (InventoryOffCooldown())
			{
				if (MeleeWeapon->CanLunge())
				{
					StartLunge(MeleeWeapon);
				}
				else
				{
					FireExtraWeapon(MeleeWeapon);
				}
			}
			
			/*if (InventoryOffCooldown())
			{
			}*/
		}
	}
}

void AShooterCharacter::OnReload()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->StartReload();
		}
	}
}


void AShooterCharacter::OnStartRunning()
{
	if (bCanRun)
	{
		AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
		if (MyPC && MyPC->IsGameInputAllowed())
		{
			if (IsTargeting())
			{
				SetTargeting(false);
			}
			StopWeaponFire();
			SetRunning(true, false);
		}
	}
}

void AShooterCharacter::OnStartRunningToggle()
{
	if (bCanRun)
	{
		AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
		if (MyPC && MyPC->IsGameInputAllowed())
		{
			if (IsTargeting())
			{
				SetTargeting(false);
			}
			StopWeaponFire();
			SetRunning(true, true);
		}
	}
}

void AShooterCharacter::OnStopRunning()
{
	if (bCanRun)
	{
		SetRunning(false, false);
	}
}

bool AShooterCharacter::IsRunning() const
{	
	if (!GetCharacterMovement())
	{
		return false;
	}

	return (bWantsToRun || bWantsToRunToggled) && !GetVelocity().IsZero() && (GetVelocity().GetSafeNormal2D() | GetActorRotation().Vector()) > -0.1;
	
}

//This function is called on every frame.
void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckLungeFinished();

	/*if (CurrentWeapon && CurrentWeapon->OffCooldown())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow, FString(TEXT("Off cooldown.")));
		}
	}*/
	

	if (bWantsToRunToggled && !IsRunning())
	{
		SetRunning(false, false);
	}

	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->HasHealthRegen())
	{
		if (this->Health < this->GetMaxHealth())
		{
			this->Health +=  5 * DeltaSeconds;
			if (Health > this->GetMaxHealth())
			{
				Health = this->GetMaxHealth();
			}
		}
	}

	if (LowHealthSound && GEngine->UseSound())
	{
		if ((this->Health > 0 && this->Health < this->GetMaxHealth() * LowHealthPercentage) && (!LowHealthWarningPlayer || !LowHealthWarningPlayer->IsPlaying()))
		{
			LowHealthWarningPlayer = UGameplayStatics::PlaySoundAttached(LowHealthSound, GetRootComponent(),
				NAME_None, FVector(ForceInit), EAttachLocation::KeepRelativeOffset, true);
			LowHealthWarningPlayer->SetVolumeMultiplier(0.0f);
		} 
		else if ((this->Health > this->GetMaxHealth() * LowHealthPercentage || this->Health < 0) && LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
		{
			LowHealthWarningPlayer->Stop();
		}
		if (LowHealthWarningPlayer && LowHealthWarningPlayer->IsPlaying())
		{
			const float MinVolume = 0.3f;
			const float VolumeMultiplier = (1.0f - (this->Health / (this->GetMaxHealth() * LowHealthPercentage)));
			LowHealthWarningPlayer->SetVolumeMultiplier(MinVolume + (1.0f - MinVolume) * VolumeMultiplier);
		}
	}
}

void AShooterCharacter::OnStartJump()
{
	AShooterPlayerController* MyPC = Cast<AShooterPlayerController>(Controller);
	if (MyPC && MyPC->IsGameInputAllowed())
	{
		if (!bLunging)
		{
			bPressedJump = true;
		}
	}
}

void AShooterCharacter::OnStopJump()
{
	bPressedJump = false;
}

//////////////////////////////////////////////////////////////////////////
// Replication

void AShooterCharacter::PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker )
{
	Super::PreReplication( ChangedPropertyTracker );

	// Only replicate this property for a short duration after it changes so join in progress players don't get spammed with fx when joining late
	DOREPLIFETIME_ACTIVE_OVERRIDE( AShooterCharacter, LastTakeHitInfo, GetWorld() && GetWorld()->GetTimeSeconds() < LastTakeHitTimeTimeout );
}

void AShooterCharacter::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	// only to local owner: weapon change requests are locally instigated, other clients don't need it
	DOREPLIFETIME_CONDITION( AShooterCharacter, Inventory,			COND_OwnerOnly );

	// everyone except local owner: flag change is locally instigated
	DOREPLIFETIME_CONDITION( AShooterCharacter, bIsTargeting,		COND_SkipOwner );
	DOREPLIFETIME_CONDITION( AShooterCharacter, bWantsToRun,		COND_SkipOwner );

	DOREPLIFETIME_CONDITION( AShooterCharacter, LastTakeHitInfo,	COND_Custom );

	// everyone
	DOREPLIFETIME( AShooterCharacter, CurrentWeapon );
	DOREPLIFETIME( AShooterCharacter, Health );

	DOREPLIFETIME(AShooterCharacter, bLunging);

	DOREPLIFETIME(AShooterCharacter, LungeActor);
}

AShooterWeapon* AShooterCharacter::GetWeapon() const
{
	return CurrentWeapon;
}

int32 AShooterCharacter::GetInventoryCount() const
{
	return Inventory.Num();
}

AShooterWeapon* AShooterCharacter::GetInventoryWeapon(int32 index) const
{
	return Inventory[index];
}

USkeletalMeshComponent* AShooterCharacter::GetPawnMesh() const
{
	return IsFirstPerson() ? Mesh1P : GetMesh();
}

USkeletalMeshComponent* AShooterCharacter::GetSpecifcPawnMesh( bool WantFirstPerson ) const
{
	return WantFirstPerson == true  ? Mesh1P : GetMesh();
}

FName AShooterCharacter::GetWeaponAttachPoint() const
{
	return WeaponAttachPoint;
}

float AShooterCharacter::GetTargetingSpeedModifier() const
{
	return TargetingSpeedModifier;
}

bool AShooterCharacter::IsTargeting() const
{
	return bIsTargeting;
}

float AShooterCharacter::GetRunningSpeedModifier() const
{
	return RunningSpeedModifier;
}

bool AShooterCharacter::IsFiring() const
{
	return bWantsToFire;
};

bool AShooterCharacter::IsFirstPerson() const
{
	return IsAlive() && Controller && Controller->IsLocalPlayerController();
}

int32 AShooterCharacter::GetMaxHealth() const
{
	return GetClass()->GetDefaultObject<AShooterCharacter>()->Health;
}

bool AShooterCharacter::IsAlive() const
{
	return Health > 0;
}

float AShooterCharacter::GetLowHealthPercentage() const
{
	return LowHealthPercentage;
}

void AShooterCharacter::UpdateTeamColorsAllMIDs()
{
	for (int32 i = 0; i < MeshMIDs.Num(); ++i)
	{
		UpdateTeamColors(MeshMIDs[i]);
	}
}
