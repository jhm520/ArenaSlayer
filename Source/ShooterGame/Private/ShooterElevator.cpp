// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterGame.h"
#include "ShooterElevator.h"


// Sets default values
AShooterElevator::AShooterElevator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AShooterElevator::ReceiveActorEndOverlap(AActor* Other)
{
	//AShooterCharacter* Pawn = Cast<AShooterCharacter>(Other);

	//if (Pawn)
	//{
	//	//UCharacterMovementComponent* PawnMove = Pawn->GetCharacterMovement();
	//	return;
	//}
	return;
}

void AShooterElevator::ReceiveActorBeginOverlap(AActor* Other)
{
	/*AShooterCharacter* Pawn = Cast<AShooterCharacter>(Other);

	if (Pawn)
	{
		Elevate(Pawn);
	}*/
	return;
}

void AShooterElevator::ElevatePawn(AShooterCharacter* Pawn)
{
	if (Role = ROLE_Authority)
	{
		UCharacterMovementComponent* PawnMove = Pawn->GetCharacterMovement();
		PawnMove->AddImpulse(ImpulseVector);
	}
	else
	{
		ServerElevatePawn(Pawn);
	}
}

void AShooterElevator::ServerElevatePawn_Implementation(AShooterCharacter* Pawn)
{
	
	ElevatePawn(Pawn);
}

bool AShooterElevator::ServerElevatePawn_Validate(AShooterCharacter* Pawn)
{
	return true;
}

// Called when the game starts or when spawned
void AShooterElevator::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AShooterElevator::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	TArray<class AActor*> OverlappingActors;

	GetOverlappingActors(OverlappingActors);

	int32 ActorsNum = OverlappingActors.Num();

	for (int32 i = 0; i < ActorsNum; i++)
	{
		AShooterCharacter* Pawn = Cast<AShooterCharacter>(OverlappingActors[i]);
		AShooterWeaponPickup* WeaponPickup = Cast<AShooterWeaponPickup>(OverlappingActors[i]);

		if (Pawn)
		{
			ElevatePawn(Pawn);
		}
	}

}

