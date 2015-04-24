// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ShooterTeleporter.generated.h"

UCLASS()
class SHOOTERGAME_API AShooterTeleporter : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AShooterTeleporter(const FObjectInitializer& ObjectInitializer);

	/** spawn inventory, setup initial variables */
	virtual void PostInitializeComponents() override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	virtual void ReceiveActorBeginOverlap(AActor* Other) override;

	UPROPERTY(EditInstanceOnly, Category = Teleporter)
	AShooterTeleporter* TpTo;

	void SendTeleport(AShooterCharacter* Pawn);

	UFUNCTION(server, reliable, WithValidation)
	void ServerSendTeleport(AShooterCharacter* Pawn);

	void ReceiveTeleport(AShooterCharacter* Pawn, AShooterTeleporter* TpFrom);

	UFUNCTION(server, reliable, WithValidation)
	void ServerReceiveTeleport(AShooterCharacter* Pawn, AShooterTeleporter* TpFrom);

	bool bTeleporting;

protected:

	UPROPERTY(EditInstanceOnly, Category = Teleporter)
	FVector TpExitVector;

	FRotator TpExitRot;
};
