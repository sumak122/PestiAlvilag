// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HorseCartModel.h"
#include "SpillLife.generated.h"

class APedestrian;
class UProceduralMeshComponent;

/**
 * What happens to a city cart's load once it is on the road (ATrafficCart::SpillOne):
 * stray cats come to lap spilt milk (AAnimals::CallToMilk); street kids run
 * up from somewhere out of sight, crouch at a coal heap, grab what they can
 * and run off with it, and the heap is gone; a beer barrel rolls on the way
 * the cart was going, slowing, bouncing back off walls, and a passer-by runs
 * after it and rolls it off home.
 */
UCLASS()
class PESTIALVILAG_API ASpillLife : public AActor
{
	GENERATED_BODY()

public:
	ASpillLife();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * A churn, sack or barrel (Lying) fell at Where, its milk, coal or beer
	 * (Puddle) round it; the cart was going Going.
	 */
	static void Spilled(const UObject* WorldContextObject, HorseCartModel::ELoad Load, UProceduralMeshComponent* Lying,
		UProceduralMeshComponent* Puddle, const FVector& Where, const FVector2D& Going);

private:
	/** A barrel rolling (or being rolled) on the road. */
	struct FBarrel
	{
		TWeakObjectPtr<UProceduralMeshComponent> Mesh;
		FVector Centre = FVector::ZeroVector;
		FVector2D Velocity = FVector2D::ZeroVector;
		/** Lying turned this way; it rolls about its own length (Axle), Angle so far (radians). */
		FQuat Lie = FQuat::Identity;
		FVector Axle = FVector::RightVector;
		double Angle = 0.0;
		/** Someone is rolling it off. */
		bool bPushed = false;
		bool bChased = false;
	};

	/** A coal heap the kids are after. */
	struct FHeap
	{
		TWeakObjectPtr<UProceduralMeshComponent> Coal;
		FVector At = FVector::ZeroVector;
		int32 Kids = 0;
	};

	enum class EStage : uint8 { Coming, Crouching, Going, Pushing };

	/** A kid after the coal, or the man after the barrel. */
	struct FHelper
	{
		TWeakObjectPtr<APedestrian> Person;
		bool bKid = true;
		EStage Stage = EStage::Coming;
		/** The heap or barrel he is after. */
		int32 Thing = INDEX_NONE;
		FVector2D Spot = FVector2D::ZeroVector;
		FVector2D Goal = FVector2D::ZeroVector;
		double GroundZ = 0.0;
		float Facing = 0.f;
		float Height = 178.f;
		float Clock = 0.f;
		float GroundClock = 0.f;
		float Unseen = 0.f;
		/** How long he crouches. */
		float Grab = 5.f;
	};

	/** Two or three kids for the heap; false when nobody could come unseen. */
	void SendKids(int32 HeapIndex);
	/** Someone to run after the barrel. */
	void SendRoller(int32 BarrelIndex);
	/** A spot on the ground Near..Far from To, out of the camera's sight, with a clear run to To. */
	bool FindHidden(const FVector& To, double Near, double Far, FVector& OutGround);
	APedestrian* MakePerson(const PersonKit::FOutfit& Dress, const FVector& Ground, float Yaw);
	void RollBarrel(FBarrel& Barrel, float DeltaSeconds);
	void PlaceBarrel(FBarrel& Barrel) const;
	/** One step of a helper; true when he is done and gone. */
	bool TickHelper(FHelper& Helper, float DeltaSeconds);
	/** Moves him towards Goal at Speed; true when there. */
	bool RunTo(FHelper& Helper, const FVector2D& Goal, float Speed, float DeltaSeconds, float ArmsOut);
	/** A way off from Here, away from the camera, clear for Reach. */
	FVector2D WayOff(const FVector2D& Here, double Reach) const;
	bool IsClear(const FVector& From, const FVector& To) const;
	void UpdateGround(FHelper& Helper, float DeltaSeconds);
	void UpdateCamera();

	TArray<FBarrel> Barrels;
	TArray<FHeap> Heaps;
	TArray<FHelper> Helpers;
	FVector Eye = FVector::ZeroVector;
	FVector EyeAhead = FVector::ForwardVector;
	double LastKids = -1000.0;
	FRandomStream Random;
};
