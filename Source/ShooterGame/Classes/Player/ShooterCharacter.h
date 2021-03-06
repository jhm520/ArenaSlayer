// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterTypes.h"
#include "ShooterCharacter.generated.h"


UCLASS(Abstract)
class AShooterCharacter : public ACharacter
{
	GENERATED_UCLASS_BODY()

	/** spawn inventory, setup initial variables */
	virtual void PostInitializeComponents() override;

	/** Update the character. (Running, health etc). */
	virtual void Tick(float DeltaSeconds) override;

	/** cleanup inventory */
	virtual void Destroyed() override;

	/** update mesh for first person  view */
	virtual void PawnClientRestart() override;

	/** [server] perform PlayerState related setup */
	virtual void PossessedBy(class AController* C) override;

	/** [client] perform PlayerState related setup */
	virtual void OnRep_PlayerState() override;

	/** 
	 * Add camera pitch to first person mesh.
	 *
	 *	@param	CameraLocation	Location of the Camera.
	 *	@param	CameraRotation	Rotation of the Camera.
	 */
	void OnCameraUpdate(const FVector& CameraLocation, const FRotator& CameraRotation);

	/** get aim offsets */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	FRotator GetAimOffsets() const;

	/** 
	 * Check if pawn is enemy if given controller.
	 *
	 * @param	TestPC	Controller to check against.
	 */
	bool IsEnemyFor(AController* TestPC) const;

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** 
	 * [server] add weapon to inventory
	 *
	 * @param Weapon	Weapon to add.
	 */
	void AddWeapon(class AShooterWeapon* Weapon);

	/** 
	 * [server] remove weapon from inventory 
	 *
	 * @param Weapon	Weapon to remove.
	 */	
	void RemoveWeapon(class AShooterWeapon* Weapon);

	/** 
	 * Find in inventory
	 *
	 * @param WeaponClass	Class of weapon to find.
	 */
	class AShooterWeapon* FindWeapon(TSubclassOf<class AShooterWeapon> WeaponClass);

	/** 
	 * [server + local] equips weapon from inventory 
	 *
	 * @param Weapon	Weapon to equip
	 */
	void EquipWeapon(class AShooterWeapon* Weapon);

	/**
	* [server + local] holsters weapon from inventory
	*
	* @param Weapon	Weapon to holster
	*/

	void HolsterWeapon(class AShooterWeapon* Weapon);

	//John
	/** [server + local]
	 *
	 *	Drop current weapon from inventory, spawn weapon pickup.
	 */
	void DropWeapon();

	/** [server + local]
	 *
	 * Interact with object
	 * ...or nothing, if nothing is there
	 */

	/**	Check if the player is pointing at an interactable object
	 *	OutObject is an optional out parameter, which the function
	 *	sets as the object the player is pointing at
	 *	OutInteraction is also an out parameter, which contains
	 *	the data for the hit result
	 */
	bool CanInteract(AActor** OutObject = NULL) const;

	void Interact();

	//John
	/*void StartQuickFire(AShooterWeapon* QuickFireWeapon);
*/
	void FireExtraWeapon(AShooterWeapon* ExtraWeapon);

	//void FinishQuickFire();

	//temp storage for QuickFire weapon
	/*AShooterWeapon* QuickFiringWeapon;

	FTimerHandle  TimerHandle_QuickFire;

	FTimerHandle TimerHandle_FinishQuickFire;*/

	//AShooterWeapon* PrevWeapon;

	//bool IsQuickFiring();

	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] starts weapon fire */
	void StartWeaponFire();

	/** [local] stops weapon fire */
	void StopWeaponFire();

	/** check if pawn can fire weapon */
	bool CanFire() const;

	/** check if pawn can reload weapon */
	bool CanReload() const;

	/** check if all inventory is off cooldown */
	bool InventoryOffCooldown(bool bExceptCurrent = false) const;

	/** [server + local] change targeting state */
	void SetTargeting(bool bNewTargeting);

	//////////////////////////////////////////////////////////////////////////
	// Movement

	/*John*/
	/*whether or not the character can run (sprint)*/
	UPROPERTY(EditDefaultsOnly, Category = CharacterMovement)
	bool bCanRun;
	/*End John*/

	/** [server + local] change running state */
	void SetRunning(bool bNewRunning, bool bToggle);
	
	//////////////////////////////////////////////////////////////////////////
	// Animations
	
	/** play anim montage */
	virtual float PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None) override;

	/** stop playing montage */
	virtual void StopAnimMontage(class UAnimMontage* AnimMontage) override;

	/** stop playing all montages */
	void StopAllAnimMontages();

	//////////////////////////////////////////////////////////////////////////
	// Input handlers

	/** setup pawn specific input handlers */
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	/** 
	 * Move forward/back
	 *
	 * @param Val Movment input to apply
	 */
	void MoveForward(float Val);

	/** 
	 * Strafe right/left 
	 *
	 * @param Val Movment input to apply
	 */	
	void MoveRight(float Val);

	/** 
	 * Move Up/Down in allowed movement modes. 
	 *
	 * @param Val Movment input to apply
	 */	
	void MoveUp(float Val);

	/* Frame rate independent turn */
	void TurnAtRate(float Val);

	/* Frame rate independent lookup */
	void LookUpAtRate(float Val);

	/** player pressed start fire action */
	void OnStartFire();

	/** player released start fire action */
	void OnStopFire();

	/** player pressed targeting action */
	void OnStartTargeting();

	/** player released targeting action */
	void OnStopTargeting();

	/** player pressed toggle targeting action */
	void OnToggleTargeting();

	/** player pressed next weapon action */
	void OnNextWeapon();

	/** player pressed prev weapon action */
	void OnPrevWeapon();

	/** Player is equipping a weapon*/
	void OnEquipWeapon(AShooterWeapon* Weapon);

	/** player pressed switch weapon action*/
	void OnSwitchWeapon();

	//John
	/**Player pressed drop weapon action*/
	void OnDropWeapon();

	/**Player pressed interact action*/
	void OnInteract();

	//John
	/**Player pressed throw frag grenade action key*/
	void OnThrowFragGrenade();

	/**Player pressed throw sticky grenade action key*/
	void OnThrowStickyGrenade();

	/**Player pressed melee action key*/
	void OnMelee();

	/** player pressed reload action */
	void OnReload();

	/** player pressed jump action */
	void OnStartJump();

	/** player released jump action */
	void OnStopJump();

	/** player pressed run action */
	void OnStartRunning();

	/** player pressed toggled run action */
	void OnStartRunningToggle();

	/** player released run action */
	void OnStopRunning();

	//////////////////////////////////////////////////////////////////////////
	// Reading data

	/** get mesh component */
	USkeletalMeshComponent* GetPawnMesh() const;

	/** get currently equipped weapon */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	class AShooterWeapon* GetWeapon() const;

	/** get weapon attach point */
	FName GetWeaponAttachPoint() const;

	/** get weapon attach point */
	FName GetWeaponHolsterPoint() const;

	/** get total number of inventory items */
	int32 GetInventoryCount() const;

	/** get player's current FOV */

	float GetFOV();

	/** 
	 * get weapon from inventory at index. Index validity is not checked.
	 *
	 * @param Index Inventory index
	 */
	class AShooterWeapon* GetInventoryWeapon(int32 index) const;

	/** get weapon taget modifier speed	*/
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	float GetTargetingSpeedModifier() const;

	/** get targeting state */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	bool IsTargeting() const;

	/** get firing state */
	UFUNCTION(BlueprintCallable, Category="Game|Weapon")
	bool IsFiring() const;

	/** get the modifier value for running speed */
	UFUNCTION(BlueprintCallable, Category=Pawn)
	float GetRunningSpeedModifier() const;

	/** get running state */
	UFUNCTION(BlueprintCallable, Category=Pawn)
	bool IsRunning() const;

	/** get camera view type */
	UFUNCTION(BlueprintCallable, Category=Mesh)
	virtual bool IsFirstPerson() const;

	/** get max health */
	int32 GetMaxHealth() const;

	/** check if pawn is still alive */
	bool IsAlive() const;

	/** returns percentage of health when low health effects should start */
	float GetLowHealthPercentage() const;

	/*
 	 * Get either first or third person mesh. 
	 *
 	 * @param	WantFirstPerson		If true returns the first peron mesh, else returns the third
	 */
	USkeletalMeshComponent* GetSpecifcPawnMesh( bool WantFirstPerson ) const;

	/** Update the team color of all player meshes. */
	void UpdateTeamColorsAllMIDs();


	/** Get maximum inventory slots*/

	int32 GetMaxInventory();

	/** Check if player's inventory is full*/

	bool InventoryFull();

	/** Check if player's inventory is full*/

	bool InventoryEmpty();


	/** Get number of primary weapons*/
	int NumPrimaryWeapons();

	///** get the originating location for quick fire damage */
	//FVector GetQuickFireOrigin();

	//bool bQuickFiring;

private:

	/** pawn mesh: 1st person view */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

protected:

	//John
	/** trigger start weapon fire from server */
	UFUNCTION(reliable, client)
	void ClientFireExtraWeapon(AShooterWeapon* ExtraWeapon);

	UFUNCTION(reliable, server, WithValidation)
		void ServerFireExtraWeapon(AShooterWeapon* ExtraWeapon);

	UFUNCTION(reliable, client)
		void ClientFireWeapon();

	//UFUNCTION(reliable, client)
	//	void ClientStopWeaponFire();

	/**The object the player is currently pointing at*/
	AActor* PointingAtObject;

	/** Drop Weapon Velocity Scalar
	 *	How fast the player can throw a weapon
	 */
	UPROPERTY(EditDefaultsOnly, Category = Pawn)
	int32 DropWeaponVelocity;

	/** Get the aim of the camera */
	FVector GetCameraAim() const;

	/** get the originating location for camera damage */
	FVector GetCameraStartLocation(const FVector& AimDir) const;

	/** get the interact hit */
	FHitResult InteractTrace(const FVector& TraceFrom, const FVector& TraceTo) const;

	/** socket or bone name for attaching weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
	FName WeaponAttachPoint;

	/** socket or bone name for attaching weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = Inventory)
		FName WeaponHolsterPoint;

	/** default inventory list */
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
	TArray<TSubclassOf<class AShooterWeapon> > DefaultInventoryClasses;

	/** weapons in inventory */
	UPROPERTY(Transient, Replicated)
	TArray<class AShooterWeapon*> Inventory;

	//John
	/** Melee weapon*/
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
		TSubclassOf<class AShooterWeapon> Melee;

	/** Frag Grenade*/
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
		TSubclassOf<class AShooterWeapon> FragGrenade;

	/** Sticky Grenade*/
	UPROPERTY(EditDefaultsOnly, Category = Inventory)
		TSubclassOf<class AShooterWeapon> StickyGrenade;

	UPROPERTY(EditDefaultsOnly, Category = Inventory)
	int32 MaxInventory;

	/** currently equipped weapon */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_CurrentWeapon)
	class AShooterWeapon* CurrentWeapon;

	/** Replicate where this pawn was last hit and damaged */
	UPROPERTY(Transient, ReplicatedUsing=OnRep_LastTakeHitInfo)
	struct FTakeHitInfo LastTakeHitInfo;

	/** Time at which point the last take hit info for the actor times out and won't be replicated; Used to stop join-in-progress effects all over the screen */
	float LastTakeHitTimeTimeout;

	/** modifier for max movement speed */
	UPROPERTY(EditDefaultsOnly, Category=Inventory)
	float TargetingSpeedModifier;

	/** current targeting state */
	UPROPERTY(Transient, Replicated)
	uint8 bIsTargeting : 1;

	//John
	/**Interact Range*/
	UPROPERTY(EditDefaultsOnly, Category = Pawn)
	float InteractRange;

	/** modifier for max movement speed */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	float RunningSpeedModifier;

	/** current running state */
	UPROPERTY(Transient, Replicated)
	uint8 bWantsToRun : 1;

	/** from gamepad running is toggled */
	uint8 bWantsToRunToggled : 1;

	/** current firing state */
	uint8 bWantsToFire : 1;

	/** currently throwing grenade */
	bool bThrowingGrenade;

	/** currently quickfiring */
	bool bQuickFiring;

	/** when low health effects should start */
	float LowHealthPercentage;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	float BaseTurnRate;

	/** Base lookup rate, in deg/sec. Other scaling may affect final lookup rate. */
	float BaseLookUpRate;

	/** material instances for setting team color in mesh (3rd person view) */
	UPROPERTY(Transient)
	TArray<UMaterialInstanceDynamic*> MeshMIDs;

	/** animation played on death */
	UPROPERTY(EditDefaultsOnly, Category=Animation)
	UAnimMontage* DeathAnim;

	/** sound played on death, local player only */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* DeathSound;

	/** effect played on respawn */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	UParticleSystem* RespawnFX;

	/** sound played on respawn */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* RespawnSound;

	/** sound played when health is low */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* LowHealthSound;

	/** sound played when running */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* RunLoopSound;

	/** sound played when stop running */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* RunStopSound;

	/** sound played when targeting state changes */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
	USoundCue* TargetingSound;

	/*John*/
	/*Sound played when player regenerates*/
	UPROPERTY(EditDefaultsOnly, Category = Health)
		USoundCue* RegenSound;

	/** used to manipulate with run loop sound */
	UPROPERTY()
	UAudioComponent* RunLoopAC;

	/** hook to looped low health sound used to stop/adjust volume */
	UPROPERTY()
	UAudioComponent* LowHealthWarningPlayer;

	/** handles sounds for running */
	void UpdateRunSounds(bool bNewRunning);

	/** handle mesh visibility and updates */
	void UpdatePawnMeshes();

	/** handle mesh colors on specified material instance */
	void UpdateTeamColors(UMaterialInstanceDynamic* UseMID);

	/** Responsible for cleaning up bodies on clients. */
	virtual void TornOff();

	/*John*/
	/*Time since last took damage*/
	float LastDamageTime;

	/*Time since last regenerate tick*/
	float LastRegenTime;

	/*If the player has started regenerating*/
	bool StartedRegen;

	/** Player's FOV */
	UPROPERTY(EditDefaultsOnly, Category=Pawn)
		float DefaultFOV;

	

	//////////////////////////////////////////////////////////////////////////
	// Damage & death

public:

	//John

	virtual void ReceiveHit
		(
		UPrimitiveComponent * MyComp,
		AActor * Other,
		UPrimitiveComponent * OtherComp,
		bool bSelfMoved,
		FVector HitLocation,
		FVector HitNormal,
		FVector NormalImpulse,
		const FHitResult & Hit
		) override;

	/** Start lunging*/
	void StartLunge(AShooterWeapon* LungingWeapon);

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerStartLunge(AShooterWeapon* LungingWeapon);

	/** Lunge in a direction*/
	void Lunge();

	/** Finish Lunge*/
	void FinishLunge();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerFinishLunge();

	void CheckLungeFinished();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerCheckLungeFinished();

	/** Is the pawn lunging?*/
	bool IsLunging();

	/** Identifies if pawn is in its dying state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Health)
	uint32 bIsDying:1;

	// Current health of the Pawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category=Health)
	float Health;

	/** Take damage, handle death */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) override;

	/**Check whether or not a DamageCauser can headshot.
		Namely, if a weapon can headshot.*/
	virtual bool CanHeadshot(class AActor* DamageCauser);

	/** Check whether or not the character should die to a headshot.*/
	UFUNCTION(BluePrintCallable, Category=Pawn)
		bool ShieldsDown();

	/** Set shields up*/
	void SetShields(bool bShieldsUp);

	/*UFUNCTION(Client, Reliable)
		void ClientSetShields(bool bShieldsUp);*/

	/** Pawn suicide */
	virtual void Suicide();

	/** Kill this pawn */
	virtual void KilledBy(class APawn* EventInstigator);

	/** Returns True if the pawn can die in the current state */
	virtual bool CanDie(float KillingDamage, FDamageEvent const& DamageEvent, AController* Killer, AActor* DamageCauser) const;

	/*Health regen boolean*/
	bool Regenerating() const;

	/*Health regen function*/
	virtual void Regenerate();

	/**
	 * Kills pawn.  Server/authority only.
	 * @param KillingDamage - Damage amount of the killing blow
	 * @param DamageEvent - Damage event of the killing blow
	 * @param Killer - Who killed this pawn
	 * @param DamageCauser - the Actor that directly caused the damage (i.e. the Projectile that exploded, the Weapon that fired, etc)
	 * @returns true if allowed
	 */
	virtual bool Die(float KillingDamage, struct FDamageEvent const& DamageEvent, class AController* Killer, class AActor* DamageCauser);

	// Die when we fall out of the world.
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	/** Called on the actor right before replication occurs */
	virtual void PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker ) override;

	/*John*/
	/*Health Regen*/

	/*If this player has regenerating health*/
	UPROPERTY(EditDefaultsOnly, Category = Health)
		bool bHealthRegen;

	/*How many points of health regen per tick*/
	UPROPERTY(EditDefaultsOnly, Category = Health)
		float RegenPerSecond;

	/*Time between regen ticks*/
	UPROPERTY(EditDefaultsOnly, Category = Health)
		float RegenTickRate;

	UPROPERTY(EditDefaultsOnly, Category = Health)
		float TimeBeforeRegen;

	/*Timerhandle for Regenerate function*/
	FTimerHandle TimerHandle_Regenerate;

	private:

		UStaticMeshComponent* Shields;

protected:

	//John

	FTimerHandle TimerHandle_Lunge;

	//Temp Storage for movement component variables
	float PrevWalkDecel;

	float PrevGroundFriction;

	float PrevGravScale;

	float PrevMaxWalkSpeed;

	bool LungeFinished;

	FHitResult LungeHit;
	
	bool bQuickFireLunge;

	AShooterWeapon* LungeWeapon;

	UPROPERTY(Replicated)
		bool bLunging;

	UPROPERTY(Replicated)
		AActor* LungeActor;
	
	FVector LungeStartLocation;
	FVector LungeFinishLocation;


	/** notification when killed, for both the server and client. */
	virtual void OnDeath(float KillingDamage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser);

	/** play effects on hit */
	virtual void PlayHit(float DamageTaken, struct FDamageEvent const& DamageEvent, class APawn* PawnInstigator, class AActor* DamageCauser);

	/** switch to ragdoll */
	void SetRagdollPhysics();

	/** sets up the replication for taking a hit */
	void ReplicateHit(float Damage, struct FDamageEvent const& DamageEvent, class APawn* InstigatingPawn, class AActor* DamageCauser, bool bKilled);

	/** play hit or death on client */
	UFUNCTION()
	void OnRep_LastTakeHitInfo();

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** updates current weapon */
	void SetCurrentWeapon(class AShooterWeapon* NewWeapon, class AShooterWeapon* LastWeapon = NULL);

	/** current weapon rep handler */
	UFUNCTION()
	void OnRep_CurrentWeapon(class AShooterWeapon* LastWeapon);

	/** [server] spawns default inventory */
	void SpawnDefaultInventory();

	UFUNCTION(reliable, server, WithValidation)
	void ServerSpawnDefaultInventory();

	/** [server] remove all weapons from inventory and destroy them */
	void DestroyInventory();

	/** equip weapon */
	UFUNCTION(reliable, server, WithValidation)
	void ServerEquipWeapon(class AShooterWeapon* NewWeapon);

	/** equip weapon */
	UFUNCTION(reliable, server, WithValidation)
	void ServerHolsterWeapon(class AShooterWeapon* NewWeapon);

	/** equip weapon */
	UFUNCTION(reliable, client)
	void ClientHolsterWeapon(class AShooterWeapon* NewWeapon);

	/** Drop weapon*/
	UFUNCTION(reliable, server, WithValidation)
	void ServerDropWeapon();

	///** Interact*/
	UFUNCTION(reliable, server, WithValidation)
	void ServerInteract();

	/** update targeting state */
	UFUNCTION(reliable, server, WithValidation)
	void ServerSetTargeting(bool bNewTargeting);
	
	/** update targeting state */
	UFUNCTION(reliable, server, WithValidation)
	void ServerSetRunning(bool bNewRunning, bool bToggle);

protected:
	/** Returns Mesh1P subobject **/
	FORCEINLINE USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
};


