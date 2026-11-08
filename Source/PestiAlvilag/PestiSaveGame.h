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

	/** Cars brought to Laci on his orders (Body * 100 + paint, see ALaciOrders). */
	UPROPERTY()
	TArray<int32> LaciCars;

	/** Dishes the player has eaten at least once, by id (FoodMenus). */
	UPROPERTY()
	TArray<FName> TriedDishes;

	/** The hidden lighters found, by number (LighterModel::All). */
	UPROPERTY()
	TArray<int32> FoundLighters;

	/** Schwimmer's bamboo baton, the first weapon (mission 5). */
	UPROPERTY()
	bool bHasBaton = false;

	/** Fight points not yet spent with Bandi. */
	UPROPERTY()
	int32 FightPoints = 0;

	/** Bandi's lessons learnt (bits, Brawl::ELesson order). */
	UPROPERTY()
	int32 Lessons = 0;

	/** Men beaten in Bandi's ring, in order. */
	UPROPERTY()
	int32 BoxingWins = 0;
};
