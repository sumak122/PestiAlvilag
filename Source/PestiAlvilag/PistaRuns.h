// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PistaRuns.generated.h"

class AHorseCart;
class AStoryPerson;
class UGoalMarkerComponent;
class URouteGuideComponent;

enum class EPistaRun : uint8
{
	None,
	ToCart,    // walk to his horse and cart and climb up
	ToPickup,  // drive to where the goods wait
	Loading,   // standing there while the porters load the crates
	Back,      // drive them back to his stall at Teleki tér
};

/**
 * Pista bácsi's cart runs, a side job after mission 4. Press E by him at
 * his stall on Teleki tér and he sends the player with his horse and cart
 * to fetch four crates from one of three places (the market hall's side
 * door, Keleti station, Schwimmer's timber yard). Stand the cart there and
 * the porters load it; bring it back and he pays for each crate that is
 * still on the bed (rough driving throws them off).
 */
UCLASS()
class PESTIALVILAG_API APistaRuns : public AActor
{
	GENERATED_BODY()

public:
	APistaRuns();

	virtual void Tick(float DeltaSeconds) override;

	bool IsRunning() const { return State != EPistaRun::None; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Pista")
	TObjectPtr<UGoalMarkerComponent> Marker;

	UPROPERTY(VisibleAnywhere, Category = "Pista")
	TObjectPtr<URouteGuideComponent> Guide;

private:
	AHorseCart* FindCart() const;
	bool IsMissionUnderway() const;
	double GroundAt(const FVector2D& At) const;
	void StartRun();
	void SayRun() const;
	void TickRun(APawn* Player, const AStoryPerson* Pista, float DeltaSeconds);
	/** Points the marker and the route at Target and shows the goal with the distance. */
	void Lead(const FVector& From, const FVector& Target, const FString& GoalHu, const FString& GoalEn);
	void StopRun();
	void Note(const FString& Line, float Seconds) const;
	void PistaSays(const FString& Hu, const FString& En) const;

	EPistaRun State = EPistaRun::None;

	UPROPERTY(Transient)
	TObjectPtr<AHorseCart> Cart;

	/** Which place the goods wait at (PistaRun::Pickups), and where. */
	int32 Pickup = 0;
	FVector PickupAt = FVector::ZeroVector;
	/** Where he stood when he gave the run (in case he is gone when the cart comes back). */
	FVector PistaAt = FVector::ZeroVector;
	float LoadClock = 0.f;
	float StoodFor = 0.f;
};
