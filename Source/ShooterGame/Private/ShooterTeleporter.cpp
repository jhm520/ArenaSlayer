// Fill out your copyright notice in the Description page of Project Settings.

#include "ShooterGame.h"


// Sets default values
AShooterTeleporter::AShooterTeleporter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//TpExitVector = GetActorLocation() + FVector(0, 0, 500);

	//TpExitVector = GetActorForwardVector() * 250;

	bTeleporting = false;

}

void AShooterTeleporter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	TpExitVector = GetActorLocation() + TpExitVector;
}

// Called when the game starts or when spawned
void AShooterTeleporter::BeginPlay()
{
	Super::BeginPlay();
	
}


void AShooterTeleporter::ReceiveActorBeginOverlap(AActor* Other)
{
	AShooterCharacter* Pawn = Cast<AShooterCharacter>(Other);

	if (Pawn)
	{
		if (!bTeleporting)
		{
			bTeleporting = true;
			SendTeleport(Pawn);
		}
		
	}
}

void AShooterTeleporter::SendTeleport(AShooterCharacter* Pawn)
{
	if (Role == ROLE_Authority)
	{
		if (TpTo)
		{
			TpTo->ReceiveTeleport(Pawn, this);
			bTeleporting = false;
		}
	}
	else
	{
		ServerSendTeleport(Pawn);
	}
}

//
void AShooterTeleporter::ReceiveTeleport(AShooterCharacter* Pawn, AShooterTeleporter* TpFrom)
{
	if (Role == ROLE_Authority)
	{
		FHitResult TpHit;
		FVector NewPlayerLoc = TpExitVector;
		//Set the player's Z location to what it is now, so that the tp doesn't make them "jump"
		NewPlayerLoc.Z = Pawn->GetActorLocation().Z;
		Pawn->SetActorLocation(NewPlayerLoc, false, &TpHit);
	}
	else
	{
		ServerReceiveTeleport(Pawn, this);
	}
}

// Called every frame
void AShooterTeleporter::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

void AShooterTeleporter::ServerSendTeleport_Implementation(AShooterCharacter* Pawn)
{
	SendTeleport(Pawn);
}

bool AShooterTeleporter::ServerSendTeleport_Validate(AShooterCharacter* Pawn)
{
	return true;
}

void AShooterTeleporter::ServerReceiveTeleport_Implementation(AShooterCharacter* Pawn, AShooterTeleporter* TpFrom)
{
	ReceiveTeleport(Pawn, this);
}

bool AShooterTeleporter::ServerReceiveTeleport_Validate(AShooterCharacter* Pawn, AShooterTeleporter* TpFrom)
{
	return true;
}