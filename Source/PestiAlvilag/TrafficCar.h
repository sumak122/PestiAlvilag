// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarBody.h"
#include "GameFramework/Actor.h"
#include "TrafficCar.generated.h"

class ATrafficManager;
class UBoxComponent;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * An AI car that follows the traffic lanes, keeps left, slows for corners
 * and stops behind whatever is in front of it. It moves without physics, so
 * the player's car bumps into it but cannot push it.
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
	int32 GetNextLaneIndex() const { return NextLaneIndex; }

	/** Called by the manager right after spawning. */
	void StartOnLane(ATrafficManager* InManager, int32 InLaneIndex, int32 InPointIndex, const FLinearColor& Paint);

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

	UPROPERTY(VisibleAnywhere, Category = "Traffic")
	TArray<TObjectPtr<UStaticMeshComponent>> BodyParts;

	/** Period turn signals: little arms that swing out of the cabin side. */
	UPROPERTY(VisibleAnywhere, Category = "Traffic")
	TObjectPtr<UStaticMeshComponent> TrafficatorLeft;

	UPROPERTY(VisibleAnywhere, Category = "Traffic")
	TObjectPtr<UStaticMeshComponent> TrafficatorRight;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	UPROPERTY(Transient)
	TObjectPtr<ATrafficManager> Manager;

	/** Speed limit from the junction ahead: full speed if free, else stop at the line. */
	float JunctionSpeedLimit(float DistToJunction, int32 Node, float DeltaSeconds);
	void ReleasePassedJunctions(const FVector& Location, int32 NextNode);
	void UpdateTrafficators(float DistToJunction);
	/** Fraction of lane speed for a bend of this many degrees. */
	static float CornerFraction(float TurnDegrees);

	int32 LaneIndex = INDEX_NONE;
	/** Chosen as soon as the car enters a lane, so it can signal the turn. */
	int32 NextLaneIndex = INDEX_NONE;
	int32 PointIndex = 0;
	float CurrentSpeed = 0.f;
	float JunctionWaitTime = 0.f;
	float BlockedTime = 0.f;
	TArray<int32> HeldNodes;
	TArray<CarBody::EPaint> BodyPaints;
};
