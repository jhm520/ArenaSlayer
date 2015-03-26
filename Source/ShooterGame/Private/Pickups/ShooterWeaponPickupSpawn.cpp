// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterGame.h"


// Sets default values
AShooterWeaponPickupSpawn::AShooterWeaponPickupSpawn()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bPendingSpawn = false;

}

// Called when the game starts or when spawned
void AShooterWeaponPickupSpawn::BeginPlay()
{
	Super::BeginPlay();
	
	if (bStartSpawned)
	{
		SpawnWeapon();
	}
}

// Called every frame
void AShooterWeaponPickupSpawn::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void AShooterWeaponPickupSpawn::OnPickupTaken()
{
	GetWorldTimerManager().SetTimer(TimerHandle_SpawnWeapon, this, &AShooterWeaponPickupSpawn::SpawnWeapon, SpawnRate, false);
}

void AShooterWeaponPickupSpawn::SpawnWeapon()
{
	if (WeaponPickupType)
	{
		if (Role == ROLE_Authority)
		{
			//Spawn this weapon
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.bNoCollisionFail = true;
			FVector NewLocation = GetActorLocation();
			FRotator NewRotator = FRotator::ZeroRotator;

			//Spawn new weapon pickup to match removed weapon
			AShooterWeaponPickup* NewPickup = GetWorld()->SpawnActor<AShooterWeaponPickup>(WeaponPickupType, NewLocation, NewRotator, SpawnInfo);

			NewPickup->AttachSpawn(this);

			GetWorldTimerManager().ClearTimer(TimerHandle_SpawnWeapon);
		}
		else
		{
			ServerSpawnWeapon();
		}
	}
}

bool AShooterWeaponPickupSpawn::ServerSpawnWeapon_Validate()
{
	return true;
}

void AShooterWeaponPickupSpawn::ServerSpawnWeapon_Implementation()
{
	SpawnWeapon();
}

