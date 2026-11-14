// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pedestrians.h"
#include "CarjackedDriver.generated.h"

/**
 * The driver of a car the player took: he jumps out, shouts after it and
 * runs away from the player, then slows to a walk and is gone after a while.
 */
UCLASS()
class PESTIALVILAG_API ACarjackedDriver : public APedestrian
{
	GENERATED_BODY()

public:
	ACarjackedDriver();

	virtual void Tick(float DeltaSeconds) override;

	/** Runs off away from Danger (the player), shouting after his car (or his bicycle). */
	void RunFrom(const FVector& Danger, bool bBicycle = false);

	/**
	 * The owner of a bicycle or cart the player took: now and then he runs
	 * after the player shouting (and fights him if he catches him on foot),
	 * or runs to the nearest constable and tells on him; otherwise he runs off
	 * as from a car. Dress is his clothes (for the fight).
	 */
	void AfterThief(const FVector& Danger, bool bBicycle, const PersonKit::FOutfit& Dress);

private:
	enum class EMode : uint8 { Away, Chase, Fetch, Fist };
	EMode Mode = EMode::Away;
	bool bBike = false;
	/** The constable he runs to, and when he last looked where he is. */
	FVector Constable = FVector::ZeroVector;
	float LookClock = 0.f;
	float ShoutClock = 0.f;
	float ModeClock = 0.f;
	PersonKit::FOutfit Clothes;
	/** One step along Heading at Speed, turning aside from walls, on the ground. */
	void Step(float Speed, float DeltaSeconds);
	/** Points Heading at To (on the map). */
	void HeadFor(const FVector& To);
	/** The chase, the run to a constable, the fist shaking: false when he is back to running off. */
	bool TickOwner(float DeltaSeconds);

	FVector2D Heading = FVector2D(1.0, 0.0);
	float Clock = 0.f;
	float GroundClock = 0.f;
	double GroundZ = 0.0;
	bool bGroundKnown = false;
};
