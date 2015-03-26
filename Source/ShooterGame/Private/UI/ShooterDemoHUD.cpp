// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Engine/DemoNetDriver.h"

#define LOCTEXT_NAMESPACE "ShooterGame.HUD.Menu"

AShooterDemoHUD::AShooterDemoHUD(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AShooterDemoHUD::DrawHUD()
{
	Super::DrawHUD();

	if ( Canvas == nullptr )
	{
		return;
	}

	UDemoNetDriver * DemoNetDriver = GetWorld()->DemoNetDriver;

	if ( DemoNetDriver == nullptr )
	{
		return;
	}

	if ( DemoNetDriver->DemoTotalTime < 0.001 )
	{
		// If there are no demo frames, we don't need to do anything
		return;
	}

	const float SizeX		= ( Canvas->SizeX * 0.70f );			// Bar is 70% of screen width
	const float SizeY		= 20.0f;
	const float X			= ( Canvas->SizeX - SizeX ) * 0.5f;		// Center on X
	const float Y			= Canvas->SizeY - 100.0f;
	const float TickPosX	= SizeX * ( DemoNetDriver->DemoCurrentTime / DemoNetDriver->DemoTotalTime );
	const float TickSizeX	= 4;
	const float TickSizeY	= SizeY + 6;

	// Draw main bar
	FCanvasTileItem TileItemMainBar( FVector2D( X, Y ), FVector2D( SizeX, SizeY ), FColor( 100, 120, 128, 128 ) );
	TileItemMainBar.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem( TileItemMainBar );

	// Draw border
	FCanvasBoxItem Border( FVector2D( X, Y ), FVector2D( SizeX, SizeY ) );

	Border.SetColor( FColor( 158, 169, 175, 255 ) );

	Border.LineThickness = 2;

	Canvas->DrawItem( Border );

	// Draw demo position
	FColor DrawColorDemoPos( 250, 250, 250, 255 );

	FCanvasTileItem TileItemDemoPos( FVector2D( X + TickPosX - TickSizeX * 0.5f, Y + ( SizeY - TickSizeY ) * 0.5f ), FVector2D( TickSizeX, TickSizeY ), DrawColorDemoPos );
	TileItemDemoPos.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem( TileItemDemoPos );

	{
		FText PlaybackText = FText::FromString( FString( TEXT( "PAUSED" ) ) );

		if ( GetWorldSettings()->Pauser == NULL )
		{
			PlaybackText = FText::FromString( FString::Printf( TEXT( "%2.2f X" ), GetWorldSettings()->DemoPlayTimeDilation ) );
		}

		float TextSizeX, TextSizeY;
		Canvas->StrLen( GEngine->GetSmallFont(), PlaybackText.ToString(), TextSizeX, TextSizeY );

		FCanvasTextItem TextItem( FVector2D( X + SizeX * 0.5f - TextSizeX * 0.5f, Y + 1 ), PlaybackText, GEngine->GetSmallFont(), FLinearColor::White );
		Canvas->DrawItem( TextItem );
	}

	{
		const int32 Minutes = DemoNetDriver->DemoCurrentTime / 60.0f;
		const int32 Seconds = DemoNetDriver->DemoCurrentTime - Minutes * 60;

		FText FrameNumText = FText::FromString( FString::Printf( TEXT( "%2i:%02i" ), Minutes, Seconds ) );

		float TextSizeX, TextSizeY;
		Canvas->StrLen( GEngine->GetSmallFont(), FrameNumText.ToString(), TextSizeX, TextSizeY );

		FCanvasTextItem TextItem( FVector2D( X - TextSizeX - 3, Y + 1 ), FrameNumText, GEngine->GetSmallFont(), FLinearColor::White );
		Canvas->DrawItem( TextItem );
	}

	{
		const int32 Minutes = DemoNetDriver->DemoTotalTime / 60.0f;
		const int32 Seconds = DemoNetDriver->DemoTotalTime - Minutes * 60;

		FText FrameNumText = FText::FromString( FString::Printf( TEXT( "%2i:%02i" ), Minutes, Seconds ) );

		FCanvasTextItem TextItem( FVector2D( X + SizeX + 3, Y + 1 ), FrameNumText, GEngine->GetSmallFont(), FLinearColor::White );
		Canvas->DrawItem( TextItem );
	}
}

#undef LOCTEXT_NAMESPACE
