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
 * tér with a beer, his uniform coat over the back of his chair and the
 * SSK's spare key in its pocket (CarShowModel::BuildTable). The player
 * stands behind the chair and holds E to lift the key while Kálmán looks
 * at the street; now and then he turns his head, and if he turns while
 * the player's hand is in the pocket he jumps up and fights
 * (AStreetFights). Beating him gets the key too; him running off, or an
 * arrest, fails the mission. The key goes to Laci.
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
	/** Kálmán's glances over his shoulder, the player's hand in the coat, and what comes of it. */
	void TickLift(float DeltaSeconds);
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
	/** How far the key is out of the pocket, 0 to 1. */
	float Lifted = 0.f;
	/** Seconds to his next glance, and how far into a glance he is (s, below 0 when not glancing). */
	float GlanceIn = 6.f;
	float Glancing = -1.f;
	/** Which way he looks round: -1 left, 1 right. */
	double GlanceSide = 1.0;
	/** Caught: the fight with him, and where it began. */
	bool bFighting = false;
	FVector FightAt = FVector::ZeroVector;
	/** The arrests counted when the lift began (one more fails the mission). */
	int32 ArrestsAtStart = 0;
	FRandomStream Random;
};
