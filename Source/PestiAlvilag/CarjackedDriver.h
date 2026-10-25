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

	/** Runs off away from Danger (the player). */
	void RunFrom(const FVector& Danger);

private:
	FVector2D Heading = FVector2D(1.0, 0.0);
	float Clock = 0.f;
	float GroundClock = 0.f;
	double GroundZ = 0.0;
	bool bGroundKnown = false;
};
