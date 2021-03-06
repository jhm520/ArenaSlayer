// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterWeapon.generated.h"


namespace EWeaponState
{
	enum Type
	{
		Idle,
		Firing,
		Reloading,
		Equipping,
	};
}

USTRUCT()
struct FWeaponData
{
	GENERATED_USTRUCT_BODY()

		/** inifite ammo for reloads */
		UPROPERTY(EditDefaultsOnly, Category = Ammo)
		bool bInfiniteAmmo;

	/** infinite ammo in clip, no reload required */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
		bool bInfiniteClip;

	/** max ammo */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)  
		int32 MaxAmmo;

	/** clip size */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
		int32 AmmoPerClip;

	/** initial clips */
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
		int32 InitialClips;

	/** Time delay before shot */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float TimeBeforeShot;

	/** time between two consecutive shots */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float TimeBetweenShots;

	/** failsafe reload duration if weapon doesn't have any animation for it */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float NoAnimReloadDuration;

	/* John's code */
	/*if this weapon is a burst/semi-automatic weapon*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		bool bBurstWeapon;

	/*how many shots per fire*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		int32 ShotsPerBurst;

	/*time delay between bursts*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float TimeBetweenBursts;

	/*if this weapon can be picked up when the player's inventory is full*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		bool bExtraWeapon;

	/* if this weapon can be equipped*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		bool bEquippable;

	/* if this weapon can be dropped*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		bool bDroppable;

	/** if this weapon is always equipped (i.e. grenades/melee)*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		bool bAlwaysEquipped;

	/** if this weapon needs to reload*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		bool bNeedsReload;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float AltEquipDuration;

	/** Does this weapon use a sphere trace instead of a line trace?*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		bool bSphereTrace;

	/** Radius of the sphere trace*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float SphereTraceRadius;

	/** The range at which the crosshair turns red*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float ReticuleRange;

	/** if this weapon has a melee lunge */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		bool bLunge;

	/** range at which this weapon can lunge */
	UPROPERTY(EditDefaultsOnly, Category=WeaponStat)
		float LungeRange;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float LungeFinishRange;

	/** Lunge impulse size */
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float LungeVelocity;

	/** Will this weapon kill instantly if it hits a player in the back?*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		bool bAssassinate;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		bool bHeadshot;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float EquipAttachTime;

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		bool AttachOnEquip;

	/** FOV when targeting with this weapon*/
	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float TargetingFOV;

	/** Second level FOV */

	UPROPERTY(EditDefaultsOnly, Category = WeaponStat)
		float TargetingFOV2;

	/*End John's code*/

	/** defaults */
	FWeaponData()
	{
		bInfiniteAmmo = false;
		bInfiniteClip = false;
		MaxAmmo = 100;
		AmmoPerClip = 20;
		InitialClips = 4;
		TimeBeforeShot = 0.0f;
		TimeBetweenShots = 0.2f;
		NoAnimReloadDuration = 1.0f;
		bBurstWeapon = false;
		ShotsPerBurst = 1;
		TimeBetweenBursts = 0.5f;
		bExtraWeapon = false;
		bEquippable = true;
		bDroppable = true;
		bNeedsReload = true;
		AltEquipDuration = 0.0f;
		bSphereTrace = false;
		SphereTraceRadius = 0.0f;
		ReticuleRange = 10000.0f;
		bLunge = false;
		LungeRange = 500.0f;
		LungeFinishRange = 150;
		LungeVelocity = 1000.0f;
		bAssassinate = false;
		bHeadshot = false;
		bAlwaysEquipped = false;
		EquipAttachTime = 0.0f;
		AttachOnEquip = false;
		TargetingFOV = 60.0f;
		TargetingFOV2 = 0.0f;
	}
};

USTRUCT()
struct FWeaponAnim
{
	GENERATED_USTRUCT_BODY()

		/** animation played on pawn (1st person view) */
		UPROPERTY(EditDefaultsOnly, Category = Animation)
		UAnimMontage* Pawn1P;

	/** animation played on pawn (3rd person view) */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
		UAnimMontage* Pawn3P;
};

UCLASS(Abstract, Blueprintable)
class AShooterWeapon : public AActor
{
	GENERATED_UCLASS_BODY()

		/** perform initial setup */
		virtual void PostInitializeComponents() override;

	virtual void Destroyed() override;

	//////////////////////////////////////////////////////////////////////////
	// Ammo

	enum class EAmmoType
	{
		EBullet,
		ERocket,
		EMax,
	};

	/** [server] add ammo */
	void GiveAmmo(int AddAmount);

	/** [server] remove ammo*/
	void TakeAmmo(int RemoveAmmount);

	/** consume a bullet */
	void UseAmmo();

	/** query ammo type */
	virtual EAmmoType GetAmmoType() const
	{
		return EAmmoType::EBullet;
	}

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** weapon is being equipped by owner pawn */
	virtual void OnEquip();

	/** weapon is now equipped by owner pawn */
	virtual void OnEquipFinished();

	/** Weapon is holstered*/
	void HolsterWeapon();

	/** weapon is holstered by owner pawn
	or being dropped*/
	virtual void OnUnEquip(bool bDropped = false);

	/** [server] weapon was added to pawn's inventory */
	virtual void OnEnterInventory(AShooterCharacter* NewOwner);

	/** [server] weapon was removed from pawn's inventory */
	virtual void OnLeaveInventory();

	/** check if it's currently equipped */
	bool IsEquipped() const;

	/** check if mesh is already attached */
	bool IsAttachedToPawn() const;


	//////////////////////////////////////////////////////////////////////////
	// Input

	/** [local + server] start weapon fire */
	virtual void StartFire();

	/** [local + server] stop weapon fire */
	virtual void StopFire();

	/** Equip this weapon without an animation */
	void QuickEquip();

	void QuickUnEquip();

	float GetTimeBetweenShots();

	/*John*/
	/*StartFire function for burst weapon*/
	void BurstWeapon_StartFire();

	/*StopFire function for burst weapon*/
	void BurstWeapon_StopFire();

	/*End John*/


	/** [all] start weapon reload */
	virtual void StartReload(bool bFromReplication = false);

	/** [local + server] interrupt weapon reload */
	virtual void StopReload();

	/** [server] performs actual reload */
	virtual void ReloadWeapon();

	/** trigger reload from server */
	UFUNCTION(reliable, client)
		void ClientStartReload();


	//////////////////////////////////////////////////////////////////////////
	// Control

	/** Set weapon equipped*/
	void SetIsEquipped(bool bEquipped);

	/** check if weapon is off cooldown and can fire*/
	bool OffCooldown() const;

	/** check if weapon can headshot*/
	bool CanHeadshot();

	/** check if weapon can assassinate*/
	bool CanAssassinate();

	/** check if weapon can lunge*/
	bool CanLunge() const;
	
	/** Get this weapon's lunge impulse size */
	float GetLungeVelocity();

	/** Get this weapon's lunge range */
	float GetLungeRange();

	/** Get this weapon's lunge finish range*/
	float GetLungeFinishRange();

	/** check if weapon can fire */
	bool CanFire() const;

	/** check if weapon can be reloaded */
	bool CanReload() const;

	/** check if, the crosshair is overlapping a target*/
	bool CanHit() const;

	/** find lunge hit */
	FHitResult LungeTrace() const;

	/** Get targeting FOV*/
	float GetTargetingFOV();

	/* Get "second leve" targeting FOV**/
	float GetTargetingFOV2();

	//////////////////////////////////////////////////////////////////////////
	// Reading data

	/** get current weapon state */
	EWeaponState::Type GetCurrentState() const;

	/** get current ammo amount (total) */
	int32 GetCurrentAmmo() const;

	/** get current ammo amount (clip) */
	int32 GetCurrentAmmoInClip() const;

	/** get clip size */
	int32 GetAmmoPerClip() const;

	/** get max ammo amount */
	int32 GetMaxAmmo() const;

	/** get weapon mesh (needs pawn owner to determine variant) */
	USkeletalMeshComponent* GetWeaponMesh() const;

	/** get pawn owner */
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
	class AShooterCharacter* GetPawnOwner() const;

	/** icon displayed on the HUD when weapon is equipped as primary */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		FCanvasIcon PrimaryIcon;

	/** icon displayed on the HUD when weapon is secondary */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		FCanvasIcon SecondaryIcon;

	/** bullet icon used to draw current clip (left side) */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		FCanvasIcon PrimaryClipIcon;

	/** bullet icon used to draw secondary clip (left side) */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		FCanvasIcon SecondaryClipIcon;

	/** how many icons to draw per clip */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		float AmmoIconsCount;

	/** defines spacing between primary ammo icons (left side) */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		int32 PrimaryClipIconOffset;

	/** defines spacing between secondary ammo icons (left side) */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		int32 SecondaryClipIconOffset;

	/** crosshair parts icons (left, top, right, bottom and center) */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		FCanvasIcon Crosshair[5];

	/** crosshair parts icons when targeting (left, top, right, bottom and center) */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		FCanvasIcon AimingCrosshair[5];

	/** only use red colored center part of aiming crosshair */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		bool UseLaserDot;

	/** false = default crosshair */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		bool UseCustomCrosshair;

	/** false = use custom one if set, otherwise default crosshair */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		bool UseCustomAimingCrosshair;

	/** true - crosshair will not be shown unless aiming with the weapon */
	UPROPERTY(EditDefaultsOnly, Category = HUD)
		bool bHideCrosshairWhileNotAiming;

	/** check if weapon has infinite ammo (include owner's cheats) */
	bool HasInfiniteAmmo() const;

	/** check if weapon has infinite clip (include owner's cheats) */
	bool HasInfiniteClip() const;

	/** set the weapon's owning pawn */
	void SetOwningPawn(AShooterCharacter* AShooterCharacter);

	/** gets last time when this weapon was switched to */
	float GetEquipStartedTime() const;

	/** gets the duration of equipping weapon*/
	float GetEquipDuration() const;

	/**	John:
	*		The Weapon Pickup of this weapon
			You should probably make this protected and make a function that returns it
	*/

	UPROPERTY(EditDefaultsOnly, Category = Pickup)
	TSubclassOf<class AShooterWeaponPickup> WeaponPickup;

	/**	Given a shooter weapon pickup, set this weapon's properties to match the pickup*/
	void SetWeaponProperties(AShooterWeaponPickup* WeaponPickup);

	/** Is this weapon an extra weapon? Can it be picked up when your inventory is full?*/
	bool IsExtraWeapon();

	/** Is this weapon equippable? */
	bool IsEquippable();

	/** Is this weapon droppable? */
	bool IsDroppable();

	/** Get this weapon's attach point name*/
	FName GetWeaponAttachPoint();

	/** Get this weapon's holster attach point*/
	FName GetWeaponHolsterPoint();

protected:

	/** pawn owner */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_MyPawn)
	class AShooterCharacter* MyPawn;

	/** weapon data */
	UPROPERTY(EditDefaultsOnly, Category = Config)
		FWeaponData WeaponConfig;

	bool bQuickFiring;

private:
	/** weapon mesh: 1st person view */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		USkeletalMeshComponent* Mesh1P;

	/** weapon mesh: 3rd person view */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		USkeletalMeshComponent* Mesh3P;
protected:

	/** firing audio (bLoopedFireSound set) */
	UPROPERTY(Transient)
		UAudioComponent* FireAC;

	/** name of bone/socket for muzzle in weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		FName MuzzleAttachPoint;

	/** name of bone/socket for Weapon in weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		FName WeaponAttachPoint;

	/** name of bone/socket for Holster in weapon mesh */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		FName WeaponHolsterPoint;

	/** FX for muzzle flash */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		UParticleSystem* MuzzleFX;

	/** spawned component for muzzle FX */
	UPROPERTY(Transient)
		UParticleSystemComponent* MuzzlePSC;

	/** spawned component for second muzzle FX (Needed for split screen) */
	UPROPERTY(Transient)
		UParticleSystemComponent* MuzzlePSCSecondary;

	/** camera shake on firing */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		TSubclassOf<UCameraShake> FireCameraShake;

	/** force feedback effect to play when the weapon is fired */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		UForceFeedbackEffect *FireForceFeedback;

	/** single fire sound (bLoopedFireSound not set) */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
		USoundCue* FireSound;

	/** looped fire sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
		USoundCue* FireLoopSound;

	/** finished burst sound (bLoopedFireSound set) */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
		USoundCue* FireFinishSound;

	/** out of ammo sound */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
		USoundCue* OutOfAmmoSound;

	/** reload sound */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
		USoundCue* ReloadSound;

	/** reload animations */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
		FWeaponAnim ReloadAnim;

	/** equip sound */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
		USoundCue* EquipSound;

	/** equip animations */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
		FWeaponAnim EquipAnim;

	/** fire animations */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
		FWeaponAnim FireAnim;

	/** is muzzle FX looped? */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
		uint32 bLoopedMuzzleFX : 1;

	/** is fire sound looped? */
	UPROPERTY(EditDefaultsOnly, Category = Sound)
		uint32 bLoopedFireSound : 1;

	/** is fire animation looped? */
	UPROPERTY(EditDefaultsOnly, Category = Animation)
		uint32 bLoopedFireAnim : 1;

	/** is fire animation playing? */
	uint32 bPlayingFireAnim : 1;

	/** is weapon currently equipped? */
	uint32 bIsEquipped : 1;

	/** is weapon fire active? */
	uint32 bWantsToFire : 1;

	/** is reload animation playing? */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Reload)
		uint32 bPendingReload : 1;

	bool bPendingShot;

	/** is equip animation playing? */
	uint32 bPendingEquip : 1;

	/** weapon is refiring */
	uint32 bRefiring;

	/*John*/

	/* weapon is bursting*/
	bool bBursting;

	/*time of last successful burst*/
	float LastBurstTime;

	/*burst start time*/
	float BurstStartTime;

	/*Time needed for a burst*/
	float BurstDuration;

	/*if a burst is pending*/
	bool bPendingBurst;

	/*if a burst pause is pending*/
	bool bPendingPause;

	/*If burst weapon is pausing*/
	bool bBurstPausing;

	/*End John*/

	/** current weapon state */
	EWeaponState::Type CurrentState;

	/** time of last successful weapon fire */
	float LastFireTime;

	/** last time when this weapon was switched to */
	float EquipStartedTime;

	/** how much time weapon needs to be equipped */
	float EquipDuration;

	/** current total ammo */
	UPROPERTY(Transient, Replicated)
		int32 CurrentAmmo;

	/** current ammo - inside clip */
	UPROPERTY(Transient, Replicated)
		int32 CurrentAmmoInClip;

	/** burst counter, used for replicating fire events to remote clients */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_BurstCounter)
		int32 BurstCounter;

	/** Handle for efficient management of OnEquipFinished timer */
	FTimerHandle TimerHandle_OnEquipFinished;

	/** Handle for efficient management of StopReload timer */
	FTimerHandle TimerHandle_StopReload;

	/** Handle for efficient management of ReloadWeapon timer */
	FTimerHandle TimerHandle_ReloadWeapon;

	/** Handle for efficient management of HandleFiring timer */
	FTimerHandle TimerHandle_HandleFiring;

	//John
	/*Start firing Timer*/
	FTimerHandle TimerHandle_StartFire;

	/*Stop fuckin firing Timer*/
	FTimerHandle TimerHandle_StopFire;

	/*Timer for delaying weapon firing*/
	FTimerHandle TimerHandle_HandleShot;

	/*Timer for attaching weapon mesh*/
	FTimerHandle TimerHandle_AttachMeshToPawn;

	/*Timer for detaching weapon mesh*/
	FTimerHandle TimerHandle_DetachMeshFromPawn;

	//////////////////////////////////////////////////////////////////////////
	// Input - server side

	UFUNCTION(reliable, server, WithValidation)
		void ServerStartFire();

	UFUNCTION(reliable, server, WithValidation)
		void ServerStopFire();

	UFUNCTION(reliable, server, WithValidation)
		void ServerStartReload();

	UFUNCTION(reliable, server, WithValidation)
		void ServerStopReload();


	//////////////////////////////////////////////////////////////////////////
	// Replication & effects

	UFUNCTION()
		void OnRep_MyPawn();

	UFUNCTION()
		void OnRep_BurstCounter();

	UFUNCTION()
		void OnRep_Reload();

	/** Called in network play to do the cosmetic fx for firing */
	virtual void SimulateWeaponFire();

	/** Called in network play to stop cosmetic fx (e.g. for a looping shot). */
	virtual void StopSimulatingWeaponFire();


	//////////////////////////////////////////////////////////////////////////
	// Weapon usage

	/** [local] weapon specific fire implementation */
	virtual void FireWeapon() PURE_VIRTUAL(AShooterWeapon::FireWeapon, );
	
	/** Fire a single shot, called within handle firing*/
	void HandleShot();

	/** [server] fire & update ammo */
	UFUNCTION(reliable, server, WithValidation)
		void ServerHandleFiring();

	/** [server] fire one shot & update ammo */
	UFUNCTION(reliable, server, WithValidation)
		void ServerHandleShot();

	/** [local + server] handle weapon fire */
	void HandleFiring();

	/** [local + server] firing started */
	virtual void OnBurstStarted();

	/** [local + server] firing finished */
	virtual void OnBurstFinished();

	/** update weapon state */
	void SetWeaponState(EWeaponState::Type NewState);

	/** determine current weapon state */
	void DetermineWeaponState();


	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** attaches weapon mesh to pawn's mesh */
	void AttachMeshToPawn();

	/** detaches weapon mesh from pawn */
	void DetachMeshFromPawn();

	UFUNCTION(Reliable, Client)
	void ClientDetachMeshFromPawn();


	//////////////////////////////////////////////////////////////////////////
	// Weapon usage helpers

	/** play weapon sounds */
	UAudioComponent* PlayWeaponSound(USoundCue* Sound);

	/** play weapon animations */
	float PlayWeaponAnimation(const FWeaponAnim& Animation);

	/** stop playing weapon animations */
	void StopWeaponAnimation(const FWeaponAnim& Animation);

	/** Get the aim of the weapon, allowing for adjustments to be made by the weapon */
	virtual FVector GetAdjustedAim() const;

	/** Get the aim of the camera */
	FVector GetCameraAim() const;

	/** get the originating location for camera damage */
	FVector GetCameraDamageStartLocation(const FVector& AimDir) const;

	/** get the muzzle location of the weapon */
	FVector GetMuzzleLocation() const;

	/** get direction of weapon's muzzle */
	FVector GetMuzzleDirection() const;

	/** find hit */
	FHitResult WeaponTrace(const FVector& TraceFrom, const FVector& TraceTo) const;

protected:
	/** Returns Mesh1P subobject **/
	FORCEINLINE USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns Mesh3P subobject **/
	FORCEINLINE USkeletalMeshComponent* GetMesh3P() const { return Mesh3P; }
};

