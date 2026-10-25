// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pedestrians.h"
#include "WalkingPerson.generated.h"

/**
 * Someone getting into or out of a taxi or a tram: walks along a few
 * points, keeping to the ground, then either stands there or is gone (into
 * the vehicle, or round the corner). Waiting people stand still until given
 * somewhere to go.
 */
UCLASS()
class PESTIALVILAG_API AWalkingPerson : public APedestrian
{
	GENERATED_BODY()

public:
	AWalkingPerson();

	virtual void Tick(float DeltaSeconds) override;

	/** Walks through these points (on the map) at Speed (cm/s); bVanish: gone at the last one. */
	void SetPath(const TArray<FVector2D>& InPath, float InSpeed, bool bInVanish);

	/** Moves the last point (where the door is now, say). */
	void SetGoal(const FVector2D& Goal);

	/** Stands still facing this way (degrees). */
	void StandFacing(double Yaw);

	/** At the end of the path (or never given one). */
	bool HasArrived() const { return Next >= Path.Num(); }

private:
	void Rest();
	/** Finds the ground under At (looking down from just above the person's middle). */
	void UpdateGround(const FVector2D& At, double MiddleZ);

	TArray<FVector2D> Path;
	int32 Next = 0;
	float Speed = 140.f;
	bool bVanish = false;
	float FacingYaw = 0.f;
	double GroundZ = 0.0;
	bool bGroundKnown = false;
	float GroundClock = 0.f;
	float Clock = 0.f;
};
