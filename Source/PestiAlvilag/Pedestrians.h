// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryPerson.h"
#include "Pedestrians.generated.h"

class AOsmGraybox;

/**
 * Someone out walking: a story person moved by APedestrians rather than by
 * his own tick, swinging his arms and legs as he goes.
 */
UCLASS()
class PESTIALVILAG_API APedestrian : public AStoryPerson
{
	GENERATED_BODY()

public:
	APedestrian();

	/** Puts him at Location (his middle) facing Yaw, walking at Speed. */
	void WalkTo(const FVector& Location, double Yaw, float Speed, float DeltaSeconds);

private:
	float Stride = 0.f;
};

/**
 * People walking the pavements around the camera. Reads the walking lines
 * ("sidewalks") from the street furniture file, keeps a crowd of walkers
 * within reach of the camera, each going along a line on the right-hand
 * side, turning onto a line that joins at its end (or turning back), and
 * moves the ones that fall far behind to a line ahead of the camera.
 */
UCLASS()
class PESTIALVILAG_API APedestrians : public AActor
{
	GENERATED_BODY()

public:
	APedestrians();

	virtual void Tick(float DeltaSeconds) override;

	/** The furniture file with the walking lines, relative to the project folder. */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	FString DataFile = TEXT("Data/OsmGraybox/street_furniture.json");

	/** How many people are about at one time. */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	int32 Crowd = 60;

	/** People further than this from the camera are moved to somewhere nearer (cm). */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	float Range = 9000.f;

	/** New people appear at least this far from the camera, out of the way (cm). */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	float AppearBeyond = 3500.f;

protected:
	virtual void BeginPlay() override;

private:
	struct FLine
	{
		TArray<FVector2D> Points;
		/** Distance along the line at each point. */
		TArray<double> Along;
		double Length = 0.0;
	};

	struct FWalker
	{
		APedestrian* Person = nullptr;
		int32 Line = 0;
		double Along = 0.0;
		int32 Dir = 1;
		float Speed = 130.f;
		float Lane = 60.f;
		double GroundZ = 0.0;
		float GroundClock = 0.f;
		bool bGroundKnown = false;
	};

	bool LoadLines();
	bool CameraLocation(FVector& OutLocation) const;
	/** Somewhere on a line near the camera, at least MinDistance away. */
	bool PickSpot(const FVector& Camera, double MinDistance, int32& OutLine, double& OutAlong);
	/** At the end of a line: on to a line that starts or ends near here, or back. */
	void TurnAtEnd(FWalker& Walker);
	FVector2D PointOn(const FLine& Line, double Along, FVector2D& OutTangent) const;
	bool GroundUnder(const FVector2D& At, double Near, double& OutZ) const;

	TArray<FLine> Lines;
	/** Line ends on a coarse grid: cell -> (line, end: 0 start, 1 end). */
	TMultiMap<FIntPoint, FIntPoint> Ends;
	TArray<int32> NearLines;
	FVector NearLinesFrom = FVector(1e9);
	TArray<FWalker> Walkers;
	FRandomStream Random;
	bool bReady = false;
};
