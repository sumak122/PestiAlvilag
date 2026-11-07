// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HorseCart.h"
#include "TrafficCart.generated.h"

class ATrafficManager;
class UProceduralMeshComponent;

/**
 * A carrier's horse and cart in the city's traffic (AHorseCart with a
 * load): the dairy's milk churns, the coal merchant's sacks or the
 * brewery's barrels, its own carter on the bench. It keeps to the traffic
 * lanes at a walk or an easy trot, and the cars, which cannot get past,
 * queue up behind it.
 *
 * Run into it hard in a car (or on anything faster than a walk) and the
 * horse bolts: it whinnies and gallops off down the street for a while with
 * the carter hauling on the reins, and churns, sacks or barrels go over the
 * side onto the road, milk, coal and beer spilling where they land. F by
 * the bench pulls the carter down: he runs off shouting, and the cart is
 * the player's to drive like Pista bácsi's.
 */
UCLASS()
class PESTIALVILAG_API ATrafficCart : public AHorseCart
{
	GENERATED_BODY()

public:
	ATrafficCart();

	virtual void Tick(float DeltaSeconds) override;
	virtual void TakeDriver(APawn* InDriver) override;
	virtual bool IsCityCart() const override { return true; }

	/** Picks the load, the horse and the carter; call before it is spawned (deferred). */
	void PickLoad(FRandomStream& Random);

	/** Called by the manager right after spawning. */
	void StartOnLane(ATrafficManager* InManager, int32 InLaneIndex, int32 InPointIndex);

	/** Still out in the traffic (not taken by the player). */
	bool IsInTraffic() const { return Manager != nullptr; }
	bool IsBolting() const { return BoltTime > 0.f; }

protected:
	virtual void BeginPlay() override;

private:
	/** Along the lane towards the next point: the pace it wants and where it steers. */
	void FollowLane(float DeltaSeconds);
	/** The player running into it: a hard knock makes the horse bolt. */
	void CheckBumps();
	void Bolt();
	/** Bolting: more of the load goes over the side, and it runs down whoever is in the way. */
	void WhileBolting(float DeltaSeconds);
	/** One churn, sack or barrel off the bed and onto the road, spilling. */
	void SpillOne();
	/** Spilt things go once they are old and out of sight; out of sight, an emptied cart is loaded again. */
	void TidySpills();
	/** The horse's sound plays only near the camera; its pieces move less often far away. */
	void UpdateNearness(float DeltaSeconds);
	/** Touching the player's box or capsule (in the ground plane), with this much room to spare. */
	bool IsTouching(const APawn* Other, double Margin) const;
	/** A line from the carter for the player to read, if he is near. */
	void CallOut(const FString& Text) const;

	UPROPERTY(Transient)
	TObjectPtr<ATrafficManager> Manager;

	/** Churns, sacks and barrels lying on the road, and the milk, coal and beer round them. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Spilled;
	TArray<double> SpilledAt;

	int32 LaneIndex = INDEX_NONE;
	int32 NextLaneIndex = INDEX_NONE;
	int32 PointIndex = 0;
	FVector Target = FVector::ZeroVector;
	/** This carter's easy pace (cm/s). */
	float Cruise = 250.f;

	/** Seconds left of the horse's bolt, till the next thing goes over, and how many went this time. */
	float BoltTime = 0.f;
	float SpillClock = 0.f;
	int32 SpilledThisBolt = 0;
	/** Held up this long (s); then it pushes through for a moment, as a stuck car does. */
	float StuckTime = 0.f;
	float SqueezeTime = 0.f;
	float BumpCooldown = 0.f;
	float FarClock = 0.f;
	bool bNear = true;
	/** The player's speed last frame: a crash may already have stopped his car this frame. */
	FVector LastPlayerVelocity = FVector::ZeroVector;
};
