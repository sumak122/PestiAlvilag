// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrafficManager.generated.h"

class ATrafficCar;
class ATram;
class FJsonObject;

/** One direction of one street piece, already offset to the left side. */
struct FTrafficLane
{
	TArray<FVector> Points;
	int32 FromNode = INDEX_NONE;
	int32 ToNode = INDEX_NONE;
	float Speed = 700.f;
	FString Way;
};

/** One tram route through the map, in driving order. Distances are in cm along the track. */
struct FTramRoute
{
	TArray<FVector> Points;
	/** Distance along the track at each point. */
	TArray<float> PointDistances;
	TArray<float> Stops;
	FString Ref;
	float Length = 0.f;
	float Speed = 700.f;

	/** Point on the track this far along it, clamped to the ends. */
	FVector LocationAt(float Distance) const;
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
	int32 NumCars = 120;

	/** How many of the cars are police patrols. */
	UPROPERTY(EditAnywhere, Category = "Traffic")
	int32 NumPolice = 6;

	/** No two cars start closer than this (cm). */
	UPROPERTY(EditAnywhere, Category = "Traffic")
	float MinSpawnSpacing = 1000.f;

	/** The debug view (key N) draws lanes, junctions and car routes within this range of the player (cm). */
	UPROPERTY(EditAnywhere, Category = "Traffic")
	float DebugRange = 20000.f;

	virtual void Tick(float DeltaSeconds) override;

	const FTrafficLane& GetLane(int32 LaneIndex) const { return Lanes[LaneIndex]; }

	/** A random lane leaving the end of this one; U-turns only at dead ends. */
	int32 PickNextLane(int32 LaneIndex) const;

	/**
	 * Distance to the nearest car, tram or player in front, in the same lane
	 * corridor. bIgnoreAICars counts only trams and the player.
	 */
	float ClearDistanceAhead(const AActor* Car, float LookAhead, bool bIgnoreAICars = false) const;

	/** True unless a car, tram or the player is within Radius of Location. */
	bool IsAreaClear(const FVector& Location, float Radius, const AActor* Asker) const;

	const FTramRoute& GetTramRoute(int32 RouteIndex) const { return TramRoutes[RouteIndex]; }

	/**
	 * Shortest way along the streets from one place to another, ignoring
	 * one-way rules (for the police). Returns false if no way is found.
	 */
	bool FindPath(const FVector& From, const FVector& To, TArray<FVector>& OutPoints) const;

	/** The lane point nearest Location that heads roughly along Forward. */
	bool FindLanePointNear(const FVector& Location, const FVector& Forward, int32& OutLane, int32& OutPoint) const;

	/** Roughly one tram per this much route length (cm); at least one per route. */
	UPROPERTY(EditAnywhere, Category = "Traffic")
	float TramSpacing = 80000.f;

	/** True unless a car (AI or player) sits within Room of the start of this lane. */
	bool HasRoomOnLane(int32 LaneIndex, float Room, const AActor* Asker) const;

	/**
	 * Asks to drive through a junction. Only one car may be in a junction at a
	 * time; plain bends in a street are always free. bForce takes the junction
	 * anyway, so two cars waiting on each other cannot get stuck for ever.
	 */
	bool TryClaimNode(int32 Node, const ATrafficCar* Car, bool bForce);
	/** True where streets meet, false where a street just bends or continues. */
	bool IsJunction(int32 Node) const;
	void ReleaseNode(int32 Node, const ATrafficCar* Car);
	FVector GetNodeLocation(int32 Node) const;

protected:
	virtual void BeginPlay() override;

private:
	bool LoadLanes(const FJsonObject& Root);
	void LoadTramRoutes(const FJsonObject& Root);
	void SpawnCars();
	void SpawnTrams();
	void DrawDebugView() const;

	bool bShowDebug = false;

	struct FNodeClaim
	{
		TWeakObjectPtr<const AActor> Holder;
		double Time = 0.0;
	};

	TArray<FTrafficLane> Lanes;
	TArray<TArray<int32>> LanesFromNode;
	TArray<FVector> NodeLocations;
	TArray<FNodeClaim> NodeClaims;

	TArray<FTramRoute> TramRoutes;

	/** Street graph for path finding: both directions of every lane. */
	struct FStreetLink
	{
		int32 Node = INDEX_NONE;
		int32 Lane = INDEX_NONE;
		bool bReverse = false;
		float Length = 0.f;
	};
	TArray<TArray<FStreetLink>> StreetLinks;
	int32 NearestNode(const FVector& Location) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATrafficCar>> Cars;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATram>> Trams;
};
