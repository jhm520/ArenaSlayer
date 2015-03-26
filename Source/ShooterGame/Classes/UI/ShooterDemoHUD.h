// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterTypes.h"
#include "ShooterDemoHUD.generated.h"

UCLASS()
class AShooterDemoHUD : public AHUD
{
	GENERATED_UCLASS_BODY()

public:

	/** Main HUD update loop. */
	virtual void DrawHUD() override;

protected:
	
};