// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"

AShooterPlayerCameraManager::AShooterPlayerCameraManager(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NormalFOV = 90.0f;
	TargetingFOV = 60.0f;
	ViewPitchMin = -87.0f;
	ViewPitchMax = 87.0f;
	bAlwaysApplyModifiers = true;
}

void AShooterPlayerCameraManager::UpdateCamera(float DeltaTime)
{
	AShooterCharacter* MyPawn = PCOwner ? Cast<AShooterCharacter>(PCOwner->GetPawn()) : NULL;
	if (MyPawn && MyPawn->IsFirstPerson())
	{
		float WeaponTargetingFOV = MyPawn->GetWeapon()->GetTargetingFOV();

		float PawnFOV = MyPawn->GetFOV();

		if (WeaponTargetingFOV > 0.0f)
		{
			TargetingFOV = WeaponTargetingFOV;
		}

		NormalFOV = PawnFOV;
		
		const float TargetFOV = MyPawn->IsTargeting() ? TargetingFOV : NormalFOV;
		DefaultFOV = FMath::FInterpTo(DefaultFOV, TargetFOV, DeltaTime, 20.0f);
	}

	Super::UpdateCamera(DeltaTime);

	if (MyPawn && MyPawn->IsFirstPerson())
	{
		MyPawn->OnCameraUpdate(GetCameraLocation(), GetCameraRotation());
	}
}
