// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Witnesses.generated.h"

class APedestrian;
class APoliceBox;
class ATrafficManager;
class UTextRenderComponent;

/**
 * Passers-by who saw the player do something the police did not: now and
 * then (rarely, and less often the better his street name) one runs to the
 * nearest police telephone box to ring the station, with a red ! over his
 * head and on the minimap. Once he has telephoned the player is wanted.
 * The player can stop him on the way: a click starts a fight with him
 * (AStreetFights), E pays him 1 pengő to forget it. A smashed box is no use
 * to him: he shakes his fist at it and goes off.
 *
 * Places that check APolicePatrols::Witnessed call Report when the police
 * did not see it; the witness is picked a moment later, on the next tick.
 */
UCLASS()
class PESTIALVILAG_API AWitnesses : public AActor
{
	GENERATED_BODY()

public:
	AWitnesses();

	virtual void Tick(float DeltaSeconds) override;

	/** Something bad the police did not see happened at Where, worth Stars once telephoned in. */
	static void Report(const UObject* WorldContextObject, const FVector& Where, int32 Stars);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Witnesses")
	TObjectPtr<USceneComponent> Root;

	/** The red ! over his head. */
	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> Mark;

	UPROPERTY(Transient)
	TObjectPtr<APedestrian> Person;

private:
	enum class EStage : uint8
	{
		None,
		Running,  // to the box, along the streets
		Phoning,  // in the box, ringing the station
		Cursing,  // the box smashed: shaking his fist at it
		Leaving,  // walking off, gone once nobody sees him
	};

	/** Picks a passer-by near Where and a box, and sets him running; false when there is none. */
	bool Start(const FVector& Where, int32 Stars);
	/** Walks off: out of the box's door if he is at one, then along the streets, going Way (or away from the box). */
	void Leave(const FVector2D& Way);
	/** He is put out of the game (fought, gone off). */
	void Finish();
	/** The player close by on foot: a click fights him, E pays him off. True when that ended it. */
	bool TickPlayer();
	/** Along Path at Speed; true at its end. */
	bool FollowPath(float Speed, float DeltaSeconds);
	void UpdateGround(float DeltaSeconds);
	FVector Middle() const;
	void Say(const TCHAR* Hu, const TCHAR* En);

	EStage Stage = EStage::None;
	TWeakObjectPtr<APoliceBox> Box;
	TWeakObjectPtr<ATrafficManager> Traffic;
	TArray<FVector2D> Path;
	int32 Next = 0;
	FVector2D Spot = FVector2D::ZeroVector;
	float Facing = 0.f;
	double GroundZ = 0.0;
	float GroundClock = 0.f;
	float Clock = 0.f;
	float UnseenClock = 0.f;
	int32 CrimeStars = 1;

	/** Something reported, waiting for the next tick. */
	bool bPending = false;
	FVector PendingAt = FVector::ZeroVector;
	int32 PendingStars = 1;
	double LastRun = -1000.0;

	FRandomStream Random;
};
