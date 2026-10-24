// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficManager.generated.h"

class ATrafficCar;

/** One direction of one street piece, already offset to the left side. */
struct FTrafficLane
{
	TArray<FVector> Points;
	int32 FromNode = INDEX_NONE;
	int32 ToNode = INDEX_NONE;
	float Speed = 700.f;
	FString Way;
};

/**
 * Reads the traffic lanes from the graybox JSON, spawns AI cars on them and
 * answers their questions: which lane to take next, and how far ahead the
 * road is clear. The game mode spawns one automatically.
 */
UCLASS()
class PESTIALVILAG_API ATrafficManager : public AActor
{
	GENERATED_BODY()

public:
	ATrafficManager();

	/** JSON file, relative to the project folder (kept out of Content so the editor does not try to import it). */
	UPROPERTY(EditAnywhere, Category = "Traffic")
	FString DataFile = TEXT("Data/OsmGraybox/blaha_test_area.json");

	UPROPERTY(EditAnywhere, Category = "Traffic")
	int32 NumCars = 40;

	/** No two cars start closer than this (cm). */
	UPROPERTY(EditAnywhere, Category = "Traffic")
	float MinSpawnSpacing = 1500.f;

	const FTrafficLane& GetLane(int32 LaneIndex) const { return Lanes[LaneIndex]; }

	/** A random lane leaving the end of this one; U-turns only at dead ends. */
	int32 PickNextLane(int32 LaneIndex) const;

	/** Distance to the nearest car (AI or player) in front, in the same lane corridor. */
	float ClearDistanceAhead(const AActor* Car, float LookAhead) const;

	/**
	 * Asks to drive through a junction. Only one car may be in a junction at a
	 * time; plain bends in a street are always free. bForce takes the junction
	 * anyway, so two cars waiting on each other cannot get stuck for ever.
	 */
	bool TryClaimNode(int32 Node, const ATrafficCar* Car, bool bForce);
	void ReleaseNode(int32 Node, const ATrafficCar* Car);
	FVector GetNodeLocation(int32 Node) const;

protected:
	virtual void BeginPlay() override;

private:
	bool LoadLanes();
	void SpawnCars();

	struct FNodeClaim
	{
		TWeakObjectPtr<const AActor> Holder;
		double Time = 0.0;
	};

	TArray<FTrafficLane> Lanes;
	TArray<TArray<int32>> LanesFromNode;
	TArray<FVector> NodeLocations;
	TArray<FNodeClaim> NodeClaims;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATrafficCar>> Cars;
};
