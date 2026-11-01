// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionZsibvasar.generated.h"

class AHorseCart;
class APestiPerson;
class AStoryPerson;
class UProceduralMeshComponent;

/**
 * Act 1, mission 4, "A zsibvásár" (The flea market). Pista bácsi's back is
 * playing up: he sends the player for six crates of goods waiting at the
 * market hall's side door on Vásár utca, with his horse and cart, which
 * stand by Tisza Kálmán tér. The player drives there, lifts the crates onto
 * the bed one by one, and drives them back to the flea market at Teleki tér.
 * Driving roughly throws crates off; they lie on the road until picked up.
 * Teaches driving the horse and cart, and carrying things.
 */
UCLASS()
class PESTIALVILAG_API AMissionZsibvasar : public AMission
{
	GENERATED_BODY()

public:
	AMissionZsibvasar();

protected:
	virtual void SetUpMission() override;
	virtual void OnStepStarted(int32 Step) override;
	virtual void OnStepDone(int32 Step) override;
	virtual void TickStep(int32 Step, float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	/** A crate standing on the ground, to be picked up. */
	void AddGroundCrate(const FVector& At, double Yaw);
	/** Picking up, carrying and lifting crates onto the cart, and those that fall off. */
	void TickCrates(int32 Step);
	/** He lets go of the crate he carries (it goes onto the cart, or nowhere). */
	void StopCarrying();
	/** The goal line and the marker: to the crate, to the cart, or on to where the step goes. */
	void UpdateGoal(int32 Step);

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Pista;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Porter;

	UPROPERTY(Transient)
	TObjectPtr<AHorseCart> Cart;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> GroundCrates;

	/** The crate in his arms, and who carries it. */
	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> HeldCrate;

	UPROPERTY(Transient)
	TObjectPtr<APestiPerson> Carrier;

	TArray<FLater> Lines;
	/** Where the cart stops by the side door, and where Pista bácsi stands. */
	FVector DoorStop = FVector::ZeroVector;
	FVector PistaAt = FVector::ZeroVector;
	/** The goal of the delivery step when no crate is astray. */
	FString DeliverHu;
	FString DeliverEn;
	bool bWarned = false;
};
