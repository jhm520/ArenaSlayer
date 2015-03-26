// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ShooterWeaponPickupSpawn.generated.h"

UCLASS()
class SHOOTERGAME_API AShooterWeaponPickupSpawn : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AShooterWeaponPickupSpawn();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	void SpawnWeapon();

	void OnPickupTaken();

protected:

	UPROPERTY(EditDefaultsOnly, Category=Spawn)
	bool bStartSpawned;

	/** which weapon pickup this is*/
	UPROPERTY(EditDefaultsOnly, Category = Weapon)
	TSubclassOf<class AShooterWeaponPickup> WeaponPickupType;

	FTimerHandle TimerHandle_SpawnWeapon;

	UPROPERTY(EditDefaultsOnly, Category = Spawn)
	float SpawnRate;

	UFUNCTION(reliable, server, WithValidation)
	void ServerSpawnWeapon();

	bool bPendingSpawn;
};
