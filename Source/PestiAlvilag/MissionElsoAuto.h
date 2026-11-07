// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionElsoAuto.generated.h"

class APestiCar;
class APoliceman;
class AStoryPerson;
class UPoliceWhistle;

/**
 * Act 1, mission 7, "Az első autó" (The first car): Laci, Bözsi néni's
 * nephew, mends cars in a yard on Szerdahelyi utca and wants Dr. Halász's
 * green Steyr, parked on Kiss József utca, where a constable walks his
 * beat up and down past it.
 *
 * Taking the car behind the constable's back makes the player wanted with
 * one star (he hears the engine), in front of him with two. With the police
 * shaken off, the Steyr goes to Laci's yard and stays over the pit. A
 * Steyr wrecked on the way, or being arrested, fails the mission; trying
 * again starts from Laci's gate with a fresh car on Kiss József utca.
 */
UCLASS()
class PESTIALVILAG_API AMissionElsoAuto : public AMission
{
	GENERATED_BODY()

public:
	AMissionElsoAuto();

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
	/** The doctor's Steyr at the kerb and the constable on his beat beside it. */
	void SpawnCarAndConstable();
	/** The constable walks up and down; once the car is taken he runs after it a little way, then stands. */
	void TickConstable(float DeltaSeconds);
	/** The moment the player drives off in the Steyr: what the constable saw. */
	void OnCarTaken();
	/** A wrecked Steyr or an arrest fails the mission; a badly knocked one gets a warning. */
	bool TickCarTrouble();

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Laci;

	UPROPERTY(Transient)
	TObjectPtr<APestiCar> Steyr;

	UPROPERTY(Transient)
	TObjectPtr<APoliceman> Constable;

	UPROPERTY(Transient)
	TObjectPtr<UPoliceWhistle> Whistle;

	TArray<FLater> Lines;
	/** The beat: its two ends (on the pavement, at the constable's middle height) and how far along he is, 0 to 1. */
	FVector BeatFrom = FVector::ZeroVector;
	FVector BeatTo = FVector::ZeroVector;
	double BeatAlong = 0.3;
	int32 BeatDir = 1;
	/** Seconds the constable still runs after the car. */
	float ChaseLeft = 0.f;
	bool bCarTaken = false;
	/** The police were after him at some point since the car was taken. */
	bool bChased = false;
	bool bWarnedWear = false;
	/** The arrests counted when the car was taken (one more fails the mission). */
	int32 ArrestsAtStart = 0;
	/** When a "lose the police first" note was last shown. */
	double LastWantedHint = -100.0;
};
