// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterWeapon_Projectile.h"

#include "ShooterProjectile.generated.h"

// 
UCLASS(Abstract, Blueprintable)
class AShooterProjectile : public AActor
{
	GENERATED_UCLASS_BODY()

	/** initial setup */
	virtual void PostInitializeComponents() override;

	/** setup velocity */
	void InitVelocity(FVector& ShootDirection);

	/** handle hit */
	UFUNCTION()
	void OnImpact(const FHitResult& HitResult);

	/** handle bounce*/
	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	/** Get whether or not this projectile is stuck */
	bool IsStuck();

private:
	/** movement component */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	UProjectileMovementComponent* MovementComp;

	/** collisions */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	USphereComponent* CollisionComp;

	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	UParticleSystemComponent* ParticleComp;
protected:

	/** effects for explosion */
	UPROPERTY(EditDefaultsOnly, Category=Effects)
	TSubclassOf<class AShooterExplosionEffect> ExplosionTemplate;

	/** controller that fired me (cache for damage calculations) */
	TWeakObjectPtr<AController> MyController;

	/** projectile data */
	struct FProjectileWeaponData WeaponConfig;

	/** did it explode? */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_Exploded)
	bool bExploded;

	/** [client] explosion happened */
	UFUNCTION()
	void OnRep_Exploded();

	/** trigger explosion */
	void Explode(const FHitResult& Impact);

	/** shutdown projectile and prepare for destruction */
	void DisableAndDestroy();

	/** update velocity on client */
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) override;

	//John
	/** TimerHandle for blowing up this projectile*/
	FTimerHandle TimerHandle_OnImpact;

	/** Trigger explosion manually*/
	void TriggerOnImpact();

	///** Server Trigger explosion*/
	UFUNCTION(reliable, server, WithValidation)
	void ServerTriggerOnImpact();

	/** Time of being shot*/
	float SpawnTime;

	//John
	/** Tick down the explode timer. Haha, tick. Like, literally a ticking bomb. =D*/
	virtual void Tick(float DeltaSeconds) override;

	/** if this projectile has bounced*/
	bool bBounced;

	/** time since last bounce*/
	float BounceTime;

	/** If this projectile is stuck to a target*/
	bool bStuck;
	
	/** Time when projectile stuck*/
	float StuckTime;

	virtual void ReceiveHit(UPrimitiveComponent * MyComp, AActor * Other, UPrimitiveComponent * OtherComp,
		bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult & Hit) override;

	UFUNCTION(Client, Reliable)
		void ClientStick(UPrimitiveComponent * MyComp, UPrimitiveComponent * OtherComp, bool bSelfMoved, FHitResult const & Hit);

	/** Sticks this projectile to the target*/
	void Stick(UPrimitiveComponent * MyComp, UPrimitiveComponent * OtherComp, bool bSelfMoved, FHitResult const & Hit);

	/** The actor this projectile is stuck to */
	AActor* StuckActor;
	

protected:
	/** Returns MovementComp subobject **/
	FORCEINLINE UProjectileMovementComponent* GetMovementComp() const { return MovementComp; }
	/** Returns CollisionComp subobject **/
	FORCEINLINE USphereComponent* GetCollisionComp() const { return CollisionComp; }
	/** Returns ParticleComp subobject **/
	FORCEINLINE UParticleSystemComponent* GetParticleComp() const { return ParticleComp; }
};