// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "UI/Menu/ShooterDemoPlaybackMenu.h"

AShooterDemoSpectator::AShooterDemoSpectator(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bTickEvenWhenPaused = true;
	bShouldPerformFullTickWhenPaused = true;
}

void AShooterDemoSpectator::SetupInputComponent()
{
	Super::SetupInputComponent();

	// UI input
	InputComponent->BindAction( "InGameMenu", IE_Pressed, this, &AShooterDemoSpectator::OnToggleInGameMenu );

	InputComponent->BindAction( "Target", IE_Pressed, this, &AShooterDemoSpectator::OnPausePlayback );

	InputComponent->BindAction( "NextWeapon", IE_Pressed, this, &AShooterDemoSpectator::OnIncreasePlaybackSpeed );
	InputComponent->BindAction( "PrevWeapon", IE_Pressed, this, &AShooterDemoSpectator::OnDecreasePlaybackSpeed );
}

void AShooterDemoSpectator::SetPlayer( UPlayer* InPlayer )
{
	Super::SetPlayer( InPlayer );

	// Build menu only after game is initialized
	ShooterDemoPlaybackMenu = MakeShareable( new FShooterDemoPlaybackMenu() );
	ShooterDemoPlaybackMenu->Construct( Cast< ULocalPlayer >( Player ) );

	FActorSpawnParameters SpawnInfo;

	SpawnInfo.Owner				= this;
	SpawnInfo.Instigator		= Instigator;
	SpawnInfo.bNoCollisionFail	= true;

	MyHUD = GetWorld()->SpawnActor<AShooterDemoHUD>( SpawnInfo );

	PlaybackSpeed = 2;
}

void AShooterDemoSpectator::OnToggleInGameMenu()
{
	// if no one's paused, pause
	if ( ShooterDemoPlaybackMenu.IsValid() )
	{
		ShooterDemoPlaybackMenu->ToggleGameMenu();
	}
}

static float PlaybackSpeedLUT[5] = { 0.1f, 0.5f, 1.0f, 2.0f, 4.0f };

void AShooterDemoSpectator::OnIncreasePlaybackSpeed()
{
	PlaybackSpeed = FMath::Clamp( PlaybackSpeed + 1, 0, 4 );

	GetWorldSettings()->DemoPlayTimeDilation = PlaybackSpeedLUT[ PlaybackSpeed ];
}

void AShooterDemoSpectator::OnDecreasePlaybackSpeed()
{
	PlaybackSpeed = FMath::Clamp( PlaybackSpeed - 1, 0, 4 );

	GetWorldSettings()->DemoPlayTimeDilation = PlaybackSpeedLUT[ PlaybackSpeed ];
}

void AShooterDemoSpectator::OnPausePlayback()
{
	AWorldSettings * WorldSettings = GetWorldSettings();

	if ( WorldSettings->Pauser == NULL )
	{
		WorldSettings->Pauser = PlayerState;
	}
	else
	{
		WorldSettings->Pauser = NULL;
	}
}
