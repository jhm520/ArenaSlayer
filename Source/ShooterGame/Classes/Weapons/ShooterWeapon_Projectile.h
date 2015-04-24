// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ShooterWeapon_Projectile.generated.h"

USTRUCT()
struct FProjectileWeaponData
{
	GENERATED_USTRUCT_BODY()

	/** projectile class */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AShooterProjectile> ProjectileClass;

	/** life time */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	float ProjectileLife;

	//John
	/** Explode timer*/
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	float ExplodeTime;
	/** life time after bounce */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	float ExplodeTimeAfterBounce;

	/** Will this grenade explode when it's velocity is zero? */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	bool ExplodeOnStop;

	/** Will this grenade stick to stickable objects? */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	bool bSticky;

	/** damage to stuck target */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
	int32 StuckDamage;

	//End John

	/** damage at impact point */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	int32 ExplosionDamage;

	/** radius of damage */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	float ExplosionRadius;

	/** type of damage */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
	TSubclassOf<UDamageType> DamageType;


	/** defaults */
	FProjectileWeaponData()
	{
		ProjectileClass = NULL;
		ProjectileLife = 10.0f;
		ExplodeTime = 0.0;
		ExplodeTimeAfterBounce = 0.0f;
		ExplosionDamage = 100;
		ExplosionRadius = 300.0f;
		ExplodeOnStop = true;
		bSticky = false;
		DamageType = UDamageType::StaticClass();
		StuckDamage = 100;
	}
};

// A weapon that fires a visible projectile
UCLASS(Abstract)
class AShooterWeapon_Projectile : public AShooterWeapon
{
	GENERATED_UCLASS_BODY()

	/** apply config on projectile */
	void ApplyWeaponConfig(FProjectileWeaponData& Data);

protected:

	virtual EAmmoType GetAmmoType() const override
	{
		return EAmmoType::ERocket;
	}

	/** weapon config */
	UPROPERTY(EditDefaultsOnly, Category=Config)
	FProjectileWeaponData ProjectileConfig;

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] weapon specific fire implementation */
	virtual void FireWeapon() override;

	/** spawn projectile on server */
	UFUNCTION(reliable, server, WithValidation)
	void ServerFireProjectile(FVector Origin, FVector_NetQuantizeNormal ShootDir);
};
