// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterGame.h"


// Sets default values
AShooterWeaponPickup::AShooterWeaponPickup(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("Mesh"));
	WeaponMesh->bAutoActivate = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateMovement = true;
	RootComponent = WeaponMesh;

}

// Called when the game starts or when spawned
void AShooterWeaponPickup::BeginPlay()
{
	Super::BeginPlay();

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.bNoCollisionFail = true;

	/**Spawn weapon to represent this */
	WeaponPickup = GetWorld()->SpawnActor<AShooterWeapon>(WeaponType, SpawnInfo);

}

// Called every frame
void AShooterWeaponPickup::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void AShooterWeaponPickup::AttachSpawn(AShooterWeaponPickupSpawn* Spawn)
{
	PickupSpawn = Spawn;
}

//John
//	Use the WeaponPickup as the pointer. It always has an instance of a weapon attached to it,
//	so it shouldn't ever reset its transient properties. Try just using
//	GetCurrentAmmo() and GetCurrentAmmoInClip() instead of setting your own properties.
void AShooterWeaponPickup::Interact(class AActor* Interactor)
{
	/*The pawn that interacted with this ShooterWeaponPickup*/
	AShooterCharacter* PickupPawn = Cast<AShooterCharacter>(Interactor);

	
	//If this character already has this weapon
	if (AShooterWeapon* PickupPawnWeapon = PickupPawn->FindWeapon(WeaponType))
	{

		//John
		//give ammo to the pawn
		int32 AmmoGiven = WeaponPickup->GetCurrentAmmo();

		const int32 MissingAmmo = FMath::Max(0, PickupPawnWeapon->GetMaxAmmo() - PickupPawnWeapon->GetCurrentAmmo());
		AmmoGiven = FMath::Min(AmmoGiven, MissingAmmo);

		//Take ammo from the weapon pickup
		WeaponPickup->TakeAmmo(AmmoGiven);

		//Give ammo to the pawn
		PickupPawnWeapon->GiveAmmo(AmmoGiven);
	}
	else
	{
		////if the player's inventory is not full
		//if (!(PickupPawn->InventoryFull()))
		//{
		//	//give the weapon to the player
		//	PickupPawn->AddWeapon(WeaponPickup);

		//	//player equips the weapon
		//	PickupPawn->EquipWeapon(WeaponPickup);

		//	//If this pickup was spawned by a ShooterWeaponPickupSpawn
		//	if (this->PickupSpawn)
		//	{
		//		//Notify its spawn that it has been taken.
		//		PickupSpawn->OnPickupTaken();
		//		PickupSpawn = NULL;
		//	}

		//	//this pickup is now gone
		//	this->Destroy();
		//}

		if (PickupPawn->InventoryFull())
		{
			PickupPawn->DropWeapon();
		}

		//give the weapon to the player
		PickupPawn->AddWeapon(WeaponPickup);

		//player equips the weapon
		PickupPawn->EquipWeapon(WeaponPickup);

		//If this pickup was spawned by a ShooterWeaponPickupSpawn
		if (this->PickupSpawn)
		{
			//Notify its spawn that it has been taken.
			PickupSpawn->OnPickupTaken();
			PickupSpawn = NULL;
		}

		//this pickup is now gone
		this->Destroy();
	}
}


//John
/**TakeDamage is called when the Pickup is interacted with*/
float AShooterWeaponPickup::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	/*If the Actor that caused the damage has the tag interact
	 * Likewise, this actor is also tagged with interact
		things that can interact, and things that are interactable will have this tag
	 */
	if (DamageCauser->ActorHasTag(FName(TEXT("Interactor"))))
	{
		Interact(DamageCauser);
	}

	return 0.0;
}

void AShooterWeaponPickup::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterWeaponPickup, CurrentAmmo);

	DOREPLIFETIME(AShooterWeaponPickup, CurrentAmmoInClip);
}

/** get current ammo amount (total) */
int32 AShooterWeaponPickup::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

/** get current ammo amount (clip) */
int32 AShooterWeaponPickup::GetCurrentAmmoInClip() const
{
	return CurrentAmmoInClip;
}

void AShooterWeaponPickup::SetWeaponPickup(AShooterWeapon* Weapon)
{

	WeaponPickup = Weapon;
	
}

