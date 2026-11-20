// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionKulcs.generated.h"

class ACafeGuest;
class AShapeSet;
class APestiCar;
class AStoryPerson;

/**
 * The car show chain, mission 2, "A kulcs" (The key): Kálmán, the car
 * importer's chauffeur, sits at a table before the Bauer tavern on Baross
 * tér, his uniform coat over the back of his chair and the SSK's spare key
 * in its pocket (CarShowModel::BuildTable). The player stands across the
 * table from him and buys him rounds with E (beer, wine, two pálinkas;
 * the glasses pile up on the table and he sways more each time); after
 * the fourth he falls asleep over the table, and E behind his chair takes
 * the key. Reaching into the coat while he is awake makes him jump up and
 * fight (AStreetFights); beating him gets the key too, him running off or
 * an arrest fails the mission. The key goes to Laci.
 */
UCLASS()
class PESTIALVILAG_API AMissionKulcs : public AMission
{
	GENERATED_BODY()

public:
	AMissionKulcs();

protected:
	virtual void SetUpMission() override;
	virtual void OnStepStarted(int32 Step) override;
	virtual void OnStepDone(int32 Step) override;
	virtual void TickStep(int32 Step, float DeltaSeconds) override;
	virtual bool GetRetrySpot(FVector& OutAt, double& OutYaw) const override;
	virtual void OnFailed() override;

private:
	/** A line someone says a few seconds from now. */
	struct FLater
	{
		double At;
		FString Who;
		FString Hu;
		FString En;
	};

	void SayLater(float Seconds, const FString& Who, const FString& LineHu, const FString& LineEn);
	/** A short note under the goal. */
	void Hint(const FString& Text) const;
	/** The table, Kálmán at it and his master's limousine at the kerb. */
	void SetTheTable();
	/** The table and its glasses, after so many rounds. */
	void LayTable();
	/** Rounds bought across the table, his swaying and sleep, the hand in his coat. */
	void TickDrinks(float DeltaSeconds);
	/** He has seen the hand in his coat: up he gets, and fights. */
	void Caught();

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Laci;

	UPROPERTY(Transient)
	TObjectPtr<ACafeGuest> Kalman;

	UPROPERTY(Transient)
	TObjectPtr<AShapeSet> TableSet;

	UPROPERTY(Transient)
	TObjectPtr<APestiCar> Limousine;

	TArray<FLater> Lines;
	/** The table's frame: its middle on the ground, +X out to the street. */
	FVector TableAt = FVector::ZeroVector;
	double TableYaw = 0.0;
	/** Rounds he has drunk on the player, and when the next can be ordered (s). */
	int32 Rounds = 0;
	double NextRoundAt = 0.0;
	/** When he nods off (s, below 0 while awake), and how far asleep he is (0 to 1). */
	double SleepsAt = -1.0;
	float Asleep = 0.f;
	/** Swaying clock (s). */
	float SwayClock = 0.f;
	/** The player was across the table last frame (Kálmán greets him once). */
	bool bAcross = false;
	/** When the "buy a round" note was last shown (s). */
	double LastNote = -100.0;
	/** Caught: the fight with him, and where it began. */
	bool bFighting = false;
	FVector FightAt = FVector::ZeroVector;
	/** The arrests counted when the drinking began (one more fails the mission). */
	int32 ArrestsAtStart = 0;
	FRandomStream Random;
};
