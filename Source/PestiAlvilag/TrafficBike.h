// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Bicycle.h"
#include "TrafficBike.generated.h"

class ATrafficManager;

/**
 * Someone of the city riding a bicycle (ABicycle) along the traffic lanes:
 * kept to the kerb side, slower than the cars, which go past. The rider
 * stops for whatever is in front, rings the bell at it, and slows at
 * junctions. Bumped, the bicycle wobbles and the rider calls out; hit hard
 * (by the player's car, bicycle, cart, or the player running into it), it
 * goes over, and the rider lies a moment, gets up cursing and rides on.
 * F beside it pulls the rider off: he runs away shouting and the player
 * rides it away like any other bicycle.
 */
UCLASS()
class PESTIALVILAG_API ATrafficBike : public ABicycle
{
	GENERATED_BODY()

public:
	ATrafficBike();

	virtual void Tick(float DeltaSeconds) override;
	virtual void TakeRider(APawn* InRider) override;

	/**
	 * Picks someone of the city to ride it and the bicycle for them (kind,
	 * colour, parcels for an errand boy); call before it is spawned (deferred).
	 */
	void PickRider(FRandomStream& Random);

	/** Called by the manager right after spawning. */
	void StartOnLane(ATrafficManager* InManager, int32 InLaneIndex, int32 InPointIndex);

	/** Still out riding in the traffic (not taken by the player). */
	bool IsInTraffic() const { return Manager != nullptr; }

protected:
	virtual void BeginPlay() override;

private:
	/** Along the lane towards the next point at the kerb side. */
	void FollowLane(float DeltaSeconds);
	/** The player, on foot or in anything, bumping into it. */
	void CheckBumps();
	/** Wobbling after a bump, or lying on its side and getting up again. */
	void Recover(float DeltaSeconds);
	void KnockOver(const FVector& From);
	/** Lying on its side, or being got up again. */
	bool IsDown() const;
	void Wobble();
	/** Where to ride for a point of the current lane: moved off it towards the kerb, short of any wall. */
	FVector KerbSpot(int32 Point) const;
	/** A line from the rider for the player to read, if he is near. */
	void CallOut(const FString& Text) const;

	UPROPERTY(Transient)
	TObjectPtr<ATrafficManager> Manager;

	int32 LaneIndex = INDEX_NONE;
	int32 NextLaneIndex = INDEX_NONE;
	int32 PointIndex = 0;
	FVector Target = FVector::ZeroVector;
	/** This rider's own easy pace (cm/s). */
	float Cruise = 450.f;
	int32 ParcelsCarried = 0;

	/** Seconds left of a wobble, and lying down; which way it went over (+1 right). */
	float WobbleTime = 0.f;
	float DownTime = 0.f;
	float FallSide = 1.f;
	/** Seconds held up by something in front; past a while it squeezes by anyway. */
	float StuckTime = 0.f;
	float SqueezeTime = 0.f;
	float BumpCooldown = 0.f;
	float BellCooldown = 0.f;
	float FarClock = 0.f;
};
