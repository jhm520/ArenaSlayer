// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Particles/ParticleSystemComponent.h"

AShooterProjectile::AShooterProjectile(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CollisionComp = ObjectInitializer.CreateDefaultSubobject<USphereComponent>(this, TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->AlwaysLoadOnClient = true;
	CollisionComp->AlwaysLoadOnServer = true;
	CollisionComp->bTraceComplexOnMove = true;
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(COLLISION_PROJECTILE);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	RootComponent = CollisionComp;

	ParticleComp = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("ParticleComp"));
	ParticleComp->bAutoActivate = false;
	ParticleComp->bAutoDestroy = false;
	ParticleComp->AttachParent = RootComponent;

	MovementComp = ObjectInitializer.CreateDefaultSubobject<UProjectileMovementComponent>(this, TEXT("ProjectileComp"));
	MovementComp->UpdatedComponent = CollisionComp;
	MovementComp->InitialSpeed = 2000.0f;
	MovementComp->MaxSpeed = 2000.0f;
	MovementComp->bRotationFollowsVelocity = true;
	MovementComp->ProjectileGravityScale = 0.f;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bReplicateMovement = true;

	SpawnTime = 0.0f;

	BounceTime = 0.0f;

	bBounced = false;
	bStuck = false;

	StuckActor = NULL;
}


void AShooterProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	CollisionComp->MoveIgnoreActors.Add(Instigator);

	//Get the weapon config
	AShooterWeapon_Projectile* OwnerWeapon = Cast<AShooterWeapon_Projectile>(GetOwner());
	if (OwnerWeapon)
	{
		OwnerWeapon->ApplyWeaponConfig(WeaponConfig);
	}

	if (WeaponConfig.ExplodeTime > 0.0f)
	{
		//Do something
	}
	else if (WeaponConfig.ExplodeOnStop)
	{
		//Projectile will explode when it stops moving
		MovementComp->OnProjectileStop.AddDynamic(this, &AShooterProjectile::OnImpact);
	}

	//Projectile will explode after a set timer
	//GetWorldTimerManager().SetTimer(TimerHandle_OnImpact, this, &AShooterProjectile::OnImpact, WeaponConfig.ExplodeTime, false);
	if (WeaponConfig.ExplodeTimeAfterBounce > 0.0f)
	{
		
	}
	if (MovementComp->bShouldBounce)
	{
		MovementComp->OnProjectileBounce.AddDynamic(this, &AShooterProjectile::OnBounce);
	}

	SpawnTime = GetWorld()->GetTimeSeconds();

	SetLifeSpan( WeaponConfig.ProjectileLife );
	MyController = GetInstigatorController();
}

void AShooterProjectile::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (WeaponConfig.ExplodeTimeAfterBounce > 0.0f)
	{
		if (!bBounced)
		{
			if (WeaponConfig.SetTimerOnFloorBounce)
			{

			}
			else
			{
				bBounced = true;
				BounceTime = GetWorld()->GetTimeSeconds();
			}
		}
	}                                  
}

void AShooterProjectile::Stick(UPrimitiveComponent * MyComp, UPrimitiveComponent * OtherComp, bool bSelfMoved, FHitResult const & Hit)
{
	if (Role == ROLE_Authority)
	{
		ClientStick(MyComp, OtherComp, bSelfMoved, Hit);
	}

	bStuck = true;
	MovementComp->StopMovementImmediately();
	MovementComp->bShouldBounce = 0;
	MovementComp->ProjectileGravityScale = 0;
	MyComp->AttachTo(OtherComp, Hit.BoneName, EAttachLocation::KeepWorldPosition);

	StuckActor = Hit.GetActor();
}

//Use OnRep_Exploded() code to recreate OnRep_Stuck(). Use the code in that function to attach the projectile to the pawn mesh
void AShooterProjectile::ReceiveHit(UPrimitiveComponent * MyComp, AActor * Other, UPrimitiveComponent * OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult & Hit)
{
	if (WeaponConfig.ExplodeTimeAfterBounce > 0.0f)
	{
		if (!bBounced)
		{
			if (WeaponConfig.SetTimerOnFloorBounce)
			{
				if (HitNormal.Z > 0.0f)
				{
					/*if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString(TEXT("Bounced off floor.")));
					}*/
					bBounced = true;
					BounceTime = GetWorld()->GetTimeSeconds();
				}
			}
		}
	}

	if (WeaponConfig.bSticky && !bStuck && OtherComp->ComponentHasTag("Sticky"))
	{
		Stick(MyComp, OtherComp, bSelfMoved, Hit);
	}
}


void AShooterProjectile::ClientStick_Implementation(UPrimitiveComponent * MyComp, UPrimitiveComponent * OtherComp, bool bSelfMoved, FHitResult const & Hit)
{
	//Replicate sticky grenade effects	
	bStuck = true;
	MovementComp->StopMovementImmediately();
	MovementComp->bShouldBounce = 0;
	MovementComp->ProjectileGravityScale = 0;
	MyComp->AttachTo(OtherComp, Hit.BoneName, EAttachLocation::KeepWorldPosition);
}


bool AShooterProjectile::IsStuck()
{
	return WeaponConfig.bSticky && bStuck;
}

void AShooterProjectile::TriggerOnImpact()
{
	if (Role == ROLE_Authority)
	{
		FVector ProjDirection = GetActorRotation().Vector();

		const FVector StartTrace = GetActorLocation() - ProjDirection * 200;
		const FVector EndTrace = GetActorLocation() + ProjDirection * 150;

		FHitResult Impact;

		if (!GetWorld()->LineTraceSingle(Impact, StartTrace, EndTrace, COLLISION_PROJECTILE, FCollisionQueryParams(TEXT("ProjClient"), true, Instigator)))
		{
			// failsafe
			Impact.ImpactPoint = GetActorLocation();
			Impact.ImpactNormal = -ProjDirection;
		}

		OnImpact(Impact);
	}
	else
	{
		ServerTriggerOnImpact();
	}
}

void AShooterProjectile::ServerTriggerOnImpact_Implementation()
{
	TriggerOnImpact();
}

bool AShooterProjectile::ServerTriggerOnImpact_Validate()
{
	return true;
}

//John
void AShooterProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	float CurrentTime = GetWorld()->GetTimeSeconds();

	if (WeaponConfig.ExplodeTime > 0.0f && CurrentTime - SpawnTime > WeaponConfig.ExplodeTime)
	{
		//blow up
		TriggerOnImpact();
	}
	else if (WeaponConfig.ExplodeTimeAfterBounce > 0.0f && bBounced && CurrentTime - BounceTime > WeaponConfig.ExplodeTimeAfterBounce)
	{
		//blow up
		TriggerOnImpact();
	}
}

void AShooterProjectile::InitVelocity(FVector& ShootDirection)
{
	if (MovementComp)
	{
		MovementComp->Velocity = ShootDirection * MovementComp->InitialSpeed;
	}
}

void AShooterProjectile::OnImpact(const FHitResult& HitResult)
{
	if (Role == ROLE_Authority)
	{
		if (!bExploded)
		{
			Explode(HitResult);
		}
	}
}

void AShooterProjectile::Explode(const FHitResult& Impact)
{
	if (ParticleComp)
	{
		ParticleComp->Deactivate();
	}

	// effects and damage origin shouldn't be placed inside mesh at impact point
	const FVector NudgedImpactLocation = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;

	if (WeaponConfig.ExplosionDamage > 0 && WeaponConfig.ExplosionRadius > 0 && WeaponConfig.DamageType)
	{
		//if this projectile stuck a player
		if (StuckActor)
		{
			TArray<AActor*> IgnoreActors;

			IgnoreActors.Add(StuckActor);
			UGameplayStatics::ApplyRadialDamage(this, WeaponConfig.ExplosionDamage, NudgedImpactLocation, WeaponConfig.ExplosionRadius, WeaponConfig.DamageType, IgnoreActors, this, MyController.Get());
			FDamageEvent DamageEvent;
			if (WeaponConfig.StuckDamage > 0)
			{
				
				StuckActor->TakeDamage(WeaponConfig.StuckDamage, DamageEvent, GetInstigatorController(), GetOwner());
			}
			else
			{
				StuckActor->TakeDamage(WeaponConfig.ExplosionDamage, DamageEvent, GetInstigatorController(), GetOwner());
			}
		}
		else
		{
			UGameplayStatics::ApplyRadialDamage(this, WeaponConfig.ExplosionDamage, NudgedImpactLocation, WeaponConfig.ExplosionRadius, WeaponConfig.DamageType, TArray<AActor*>(), this, MyController.Get());
		}

	}

	if (ExplosionTemplate)
	{
		const FRotator SpawnRotation = Impact.ImpactNormal.Rotation();

		AShooterExplosionEffect* EffectActor = GetWorld()->SpawnActorDeferred<AShooterExplosionEffect>(ExplosionTemplate, NudgedImpactLocation, SpawnRotation);
		if (EffectActor)
		{
			EffectActor->SurfaceHit = Impact;
			UGameplayStatics::FinishSpawningActor(EffectActor, FTransform(SpawnRotation, NudgedImpactLocation));
		}
	}

	bExploded = true;

	//John
	//PutDisableAndDestroy call here to correct replication issues
	DisableAndDestroy();
}

void AShooterProjectile::DisableAndDestroy()
{
	UAudioComponent* ProjAudioComp = FindComponentByClass<UAudioComponent>();

	UStaticMeshComponent* ProjStaticMeshComp = FindComponentByClass<UStaticMeshComponent>();

	if (ProjAudioComp && ProjAudioComp->IsPlaying())
	{
		ProjAudioComp->FadeOut(0.1f, 0.f);
	}

	if (ProjStaticMeshComp)
	{
		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString(TEXT("StaticMeshComponent destroyed.")));
		}*/
		ProjStaticMeshComp->DestroyComponent();
	}

	MovementComp->StopMovementImmediately();

	// give clients some time to show explosion
	SetLifeSpan( 2.0f );
}

///CODE_SNIPPET_START: AActor::GetActorLocation AActor::GetActorRotation
void AShooterProjectile::OnRep_Exploded()
{
	FVector ProjDirection = GetActorRotation().Vector();

	const FVector StartTrace = GetActorLocation() - ProjDirection * 200;
	const FVector EndTrace = GetActorLocation() + ProjDirection * 150;
	FHitResult Impact;
	
	if (!GetWorld()->LineTraceSingle(Impact, StartTrace, EndTrace, COLLISION_PROJECTILE, FCollisionQueryParams(TEXT("ProjClient"), true, Instigator)))
	{
		// failsafe
		Impact.ImpactPoint = GetActorLocation();
		Impact.ImpactNormal = -ProjDirection;
	}

	Explode(Impact);
}
///CODE_SNIPPET_END

void AShooterProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	if (MovementComp)
	{
		MovementComp->Velocity = NewVelocity;
	}
}

void AShooterProjectile::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	
	DOREPLIFETIME( AShooterProjectile, bExploded );

}