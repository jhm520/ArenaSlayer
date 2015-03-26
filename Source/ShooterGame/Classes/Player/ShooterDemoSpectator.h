// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterDemoSpectator.generated.h"

UCLASS(config=Game)
class AShooterDemoSpectator : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:
	/** shooter in-game menu */
	TSharedPtr<class FShooterDemoPlaybackMenu> ShooterDemoPlaybackMenu;

	virtual void SetupInputComponent() override;
	virtual void SetPlayer( UPlayer* Player ) override;

	void OnToggleInGameMenu();
	void OnIncreasePlaybackSpeed();
	void OnDecreasePlaybackSpeed();
	void OnPausePlayback();

	int32 PlaybackSpeed;
};

