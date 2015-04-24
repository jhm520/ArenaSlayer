// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "ShooterElevator.generated.h"

UCLASS()
class SHOOTERGAME_API AShooterElevator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AShooterElevator();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	virtual void ReceiveActorBeginOverlap(AActor* Other) override;

	virtual void ReceiveActorEndOverlap(AActor* Other) override;

	void ElevatePawn(AShooterCharacter* Pawn);

	UFUNCTION(server, reliable, WithValidation)
	void ServerElevatePawn(AShooterCharacter* Pawn);

	UPROPERTY(EditInstanceOnly, Category = Elevator)
	FVector ImpulseVector;

};
