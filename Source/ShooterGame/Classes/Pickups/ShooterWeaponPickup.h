// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ShooterWeaponPickup.generated.h"


//I had to declare the AShooterWeaponPickupSpawn class up here to make it work.
class AShooterWeaponPickupSpawn;

UCLASS()
class SHOOTERGAME_API AShooterWeaponPickup : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AShooterWeaponPickup(const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	/** This function will call when a Pawn interacts with the pickup,
	it won't actually deal any damage to the Shooter Weapon*/
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	//John
	/** weapon mesh */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* WeaponMesh;

	/** what kind of weapon this is*/
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	TSubclassOf<class AShooterWeapon> WeaponType;

	/** the weapon itself*/
	AShooterWeapon* WeaponPickup;

	/** get current ammo amount (total) */
	int32 GetCurrentAmmo() const;

	/** get current ammo amount (clip) */
	int32 GetCurrentAmmoInClip() const;

	/** Set up weapon properties (like ammo count) to be like a given weapon*/
	void SetWeaponPickup(AShooterWeapon* Weapon);

	void AttachSpawn(AShooterWeaponPickupSpawn* Spawn);


protected:
	void Interact(class AActor* Interactor);

	/** current total ammo */
	UPROPERTY(Replicated)
	int32 CurrentAmmo;

	/** current ammo - inside clip */
	UPROPERTY(Replicated)
	int32 CurrentAmmoInClip;

	AShooterWeaponPickupSpawn* PickupSpawn;

};
