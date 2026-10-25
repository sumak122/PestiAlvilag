// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PestiSaveGame.generated.h"

/**
 * What the game remembers between sessions. The player saves by sleeping at
 * home (Bözsi néni's house), so a loaded game starts at its door.
 */
UCLASS()
class PESTIALVILAG_API UPestiSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** The one save slot on disk (Saved/SaveGames/PestiAlvilag.sav). */
	static const TCHAR* SlotName();

	/** The player's money in fillér. */
	UPROPERTY()
	int32 Money = 0;

	/** Missions finished, by name. */
	UPROPERTY()
	TArray<FName> CompletedMissions;

	/** Taxi fares the player has driven for the company. */
	UPROPERTY()
	int32 TaxiJobs = 0;
};
