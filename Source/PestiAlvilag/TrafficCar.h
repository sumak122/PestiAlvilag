// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarModel.h"
#include "GameFramework/Actor.h"
#include "TrafficCar.generated.h"

class ATrafficManager;
struct FTrafficLane;
class UBoxComponent;
class UCarVisuals;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * An AI car that follows the traffic lanes, keeps left, slows for corners
 * and stops behind whatever is in front of it. It moves without physics, but
 * a crash knocks it sliding and spinning (TakeHit) until it stops and drives on.
 */
UCLASS()
class PESTIALVILAG_API ATrafficCar : public AActor
{
	GENERATED_BODY()

public:
	ATrafficCar();

	virtual void Tick(float DeltaSeconds) override;

	int32 GetLaneIndex() const { return LaneIndex; }
	int32 GetPointIndex() const { return PointIndex; }
	/** How fast it drives now, forwards (cm/s). */
	float GetDrivingSpeed() const { return CurrentSpeed; }
	int32 GetNextLaneIndex() const { return NextLaneIndex; }

	/** Called by the manager right after spawning. */
	void StartOnLane(ATrafficManager* InManager, int32 InLaneIndex, int32 InPointIndex, const FLinearColor& Paint);

	/** The colour it was painted. */
	const FLinearColor& GetPaint() const { return PaintColor; }

	/** What it looks like: the kind of car, its paint and wheels. */
	const CarModel::FStyle& GetStyle() const { return Style; }

	/** Out on the other side of the street, passing a horse cart: oncoming traffic slows for it. */
	bool IsPassing() const { return PassShift > 50.f; }

	/**
	 * Shoved by a crash: its speed changes by Push (cm/s) and it spins by Spin
	 * (degrees a second, + turns right). It slides and spins to a stop on its
	 * tyres, knocking into whatever is in the way, then drives on.
	 */
	void TakeHit(const FVector& Push, float Spin);
	/** Still sliding from a crash (not driving). */
	bool IsKnocked() const { return bKnocked; }
	/** Its body, dents and lost parts (handed on to the player's car when it is stolen). */
	UCarVisuals* GetVisuals() const;
	/** How it moves just now (cm/s), driving or knocked about. */
	FVector GetMoveVelocity() const;
	/** How heavy it is (kg), by its kind of car (CarHandling). */
	float GetMass() const;

	/** Lets go of the junctions it holds, before it leaves the traffic for good. */
	void LeaveTraffic() { ReleaseAllJunctions(); }

	/** Stops and waits where it is for this long (s), e.g. for someone hailing a taxi. */
	void HoldStill(float Seconds) { HoldTime = FMath::Max(HoldTime, Seconds); }

	/** Someone sits in the back for this long (s), then is gone (set down out of sight). */
	void CarryPassenger(const PersonKit::FOutfit& Outfit, float Seconds);

	/** Degrees per second the car can turn at cruising speed. */
	UPROPERTY(EditAnywhere, Category = "Traffic")
	float TurnRate = 110.f;

	UPROPERTY(EditAnywhere, Category = "Traffic")
	float Acceleration = 500.f;

	UPROPERTY(EditAnywhere, Category = "Traffic")
	float BrakeDeceleration = 1500.f;

	/** Centre-to-centre gap at which the car stops behind another (cm). */
	UPROPERTY(EditAnywhere, Category = "Traffic")
	float StopGap = 650.f;

	/** Seconds to wait at a busy junction before going anyway. */
	UPROPERTY(EditAnywhere, Category = "Traffic")
	float MaxJunctionWait = 4.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Traffic")
	TObjectPtr<UBoxComponent> CollisionBox;

	/** The body and wheels (see CarModel), turning wheels and exhaust smoke. */
	UPROPERTY(VisibleAnywhere, Category = "Traffic")
	TObjectPtr<UCarVisuals> Visuals;

	/** Period turn signals: little arms that swing out of the cabin side. */
	UPROPERTY(VisibleAnywhere, Category = "Traffic")
	TObjectPtr<UStaticMeshComponent> TrafficatorLeft;

	UPROPERTY(VisibleAnywhere, Category = "Traffic")
	TObjectPtr<UStaticMeshComponent> TrafficatorRight;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(Transient)
	TObjectPtr<ATrafficManager> Manager;

	/** Picks up lane driving again from this lane point (e.g. after a police chase). */
	void ResumeOnLane(int32 InLaneIndex, int32 InPointIndex);
	void ReleaseAllJunctions();
	/** While knocked by a crash: slides, spins and slows on its own. True while still knocked (skip the driving). */
	bool TickKnocked(float DeltaSeconds);

	int32 LaneIndex = INDEX_NONE;
	/** Chosen as soon as the car enters a lane, so it can signal the turn. */
	int32 NextLaneIndex = INDEX_NONE;
	int32 PointIndex = 0;
	float CurrentSpeed = 0.f;
	float BlockedTime = 0.f;
	float HoldTime = 0.f;
	/** Picks the car's look for this paint: any kind of car, unless it is a taxi or the police. */
	virtual CarModel::FStyle PickStyle(const FLinearColor& Paint) const;

private:

	/** Speed limit from the junction ahead: full speed if free, else stop at the line. */
	float JunctionSpeedLimit(float DistToJunction, int32 Node, float DeltaSeconds);
	void ReleasePassedJunctions(const FVector& Location, int32 NextNode);
	void UpdateTrafficators(float DistToJunction);
	/** Fraction of lane speed for a bend of this many degrees. */
	static float CornerFraction(float TurnDegrees);
	/**
	 * Behind a slow horse cart for a while on a two-way street, the car pulls
	 * out to the other side when nothing is coming, drives past and pulls back
	 * in; it backs off if something comes before it is level with the cart.
	 * Sets PassShift, how far to the right of its lane it drives (cm).
	 */
	void UpdatePassing(const FVector& Location, const FVector& LaneDir, const FTrafficLane& Lane, float Clear, float DeltaSeconds);

	float JunctionWaitTime = 0.f;
	/** The cart it is passing, how long it has been held up behind one, and how far out it drives. */
	TWeakObjectPtr<const AActor> PassingCart;
	float BehindCartTime = 0.f;
	float PassingTime = 0.f;
	float PassShift = 0.f;
	float PassShiftWanted = 0.f;
	float PassGap = 0.f;
	/** Knocked by a crash: how it slides (cm/s) and spins (degrees a second), and how long it has lain still. */
	bool bKnocked = false;
	FVector KnockVelocity = FVector::ZeroVector;
	float KnockSpin = 0.f;
	float KnockStill = 0.f;
	/** Seconds the passenger in the back still rides. */
	float PassengerTime = 0.f;
	TArray<int32> HeldNodes;
	FLinearColor PaintColor = FLinearColor::Black;
	CarModel::FStyle Style;
};

/** A taxi out on its rounds: the saloon in black with the taxi markings (see APestiTaxi). */
UCLASS()
class PESTIALVILAG_API ATrafficTaxi : public ATrafficCar
{
	GENERATED_BODY()

public:
	ATrafficTaxi();

protected:
	virtual CarModel::FStyle PickStyle(const FLinearColor& Paint) const override;
};
