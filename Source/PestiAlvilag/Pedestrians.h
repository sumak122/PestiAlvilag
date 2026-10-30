// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryPerson.h"
#include "Pedestrians.generated.h"

class AOsmGraybox;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

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

	/**
	 * Puts him at Location (his middle) facing Yaw, walking at Speed. With
	 * ArmsUp above 0 he throws his arms up in fright as he goes.
	 */
	void WalkTo(const FVector& Location, double Yaw, float Speed, float DeltaSeconds, float ArmsUp = 0.f);

	/**
	 * Holds him still in a pose: tipped back by Rotation's pitch (90 is lying
	 * on his back), thighs forward by Legs, knees bent by Knees (degrees) and
	 * arms raised forward by Arms.
	 */
	void Hold(const FVector& Location, const FRotator& Rotation, float Legs, float Knees, float Arms);

private:
	float Stride = 0.f;
};

/**
 * People walking the pavements around the camera. Reads the walking lines
 * ("sidewalks") from the street furniture file, keeps a crowd of walkers
 * within reach of the camera, each going along a line on the right-hand
 * side, turning onto a line that joins at its end (or turning back), and
 * moves the ones that fall far behind, once nobody can see them, to a line
 * nearer the camera. After the first moment, people only appear out of the
 * camera's view or behind a building, and they never jump: at a corner or
 * a turn they walk over to the next line.
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
	int32 Crowd = 90;

	/** Nobody new appears closer than this to someone already about (cm), so they spread out. */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	float Spacing = 600.f;

	/** People further than this from the camera are moved to somewhere nearer (cm). */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	float Range = 9000.f;

	/** New people appear at least this far from the camera, out of the way (cm). */
	UPROPERTY(EditAnywhere, Category = "Pedestrians")
	float AppearBeyond = 3500.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BloodPaint;

private:
	/**
	 * What he is doing besides walking: stepping out of the way of someone
	 * running, stumbling back after a bump, leaping clear of a car, running
	 * off from a car on the pavement, knocked down (and getting up again), or
	 * run over at speed and lying still.
	 */
	enum class EReact : uint8 { None, Aside, Stumble, Leap, Flee, Down, GetUp, Dead };

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
		/** Where he is drawn: follows his place on the line, without jumps. */
		FVector2D Shown = FVector2D::ZeroVector;
		float ShownYaw = 0.f;
		bool bPlaced = false;
		EReact React = EReact::None;
		/** Seconds left in what he is doing now, and how long since it began. */
		float ReactLeft = 0.f;
		float ReactTime = 0.f;
		/** How fast he is moved off his walk while reacting (cm/s). */
		FVector2D Shove = FVector2D::ZeroVector;
		/** Not before this time does he notice a car coming (he may freeze instead). */
		float NoticeAt = -1.f;
		bool bFrozen = false;
		/** The blood under him once he is run over. */
		UStaticMeshComponent* Blood = nullptr;
	};

	bool LoadLines();
	bool CameraView(FVector& OutLocation, FVector2D& OutLooking) const;
	/**
	 * Somewhere on a line near the camera, at least MinDistance away, with
	 * the ground under it; unless bInView, only out of the camera's view.
	 */
	bool PickSpot(const FVector& Camera, const FVector2D& Looking, bool bInView, double MinDistance,
		int32& OutLine, double& OutAlong, double& OutGroundZ);
	/** At the end of a line: on to a line that starts or ends near here, or back. */
	void TurnAtEnd(FWalker& Walker);
	FVector2D PointOn(const FLine& Line, double Along, FVector2D& OutTangent) const;
	/** True when a building stands between the camera and someone standing here. */
	bool BehindBuilding(const FVector& Camera, const FVector2D& At, double GroundZ) const;
	bool GroundUnder(const FVector2D& At, double Near, double& OutZ) const;
	/** Sees how the player, on foot or at the wheel, comes past him, and has him react. */
	void ReactToPlayer(FWalker& Walker, double Now);
	void StartReact(FWalker& Walker, EReact React, float Seconds, const FVector2D& Shove);
	/** The car hit him: knocked down, or killed at speed. */
	void RunOver(FWalker& Walker, double Speed);
	/** Has him pose as what he is doing now demands; false when just walking. */
	bool PoseReacting(FWalker& Walker, float DeltaSeconds);
	/** Back to walking (and the blood gone), once he is cleared away. */
	void ClearReact(FWalker& Walker);
	/** Someone calls out (only now and then, and not over other speech). */
	void Shout(const TCHAR* Hu, const TCHAR* En);

	TArray<FLine> Lines;
	/** Line ends on a coarse grid: cell -> (line, end: 0 start, 1 end). */
	TMultiMap<FIntPoint, FIntPoint> Ends;
	TArray<int32> NearLines;
	/** The longest of the near lines (people are spread along the lines by their length). */
	double NearLongest = 1.0;
	FVector NearLinesFrom = FVector(1e9);
	TArray<FWalker> Walkers;
	FRandomStream Random;
	bool bReady = false;

	/** The player (or the car he drives) and how fast it goes, worked out each frame. */
	TWeakObjectPtr<AActor> LastPlayer;
	FVector LastPlayerAt = FVector::ZeroVector;
	FVector PlayerVelocity = FVector::ZeroVector;
	FVector PlayerAt = FVector::ZeroVector;
	FVector2D PlayerForward = FVector2D(1.0, 0.0);
	bool bPlayerDriving = false;
	double LastShout = -10.0;
};
