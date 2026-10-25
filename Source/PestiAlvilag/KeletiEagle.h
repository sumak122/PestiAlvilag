// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KeletiEagle.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;

/**
 * A golden eagle over Keleti station, a quiet nod to Assassin's Creed
 * Mirage. It circles high above the hall, gliding with now and then a few
 * wingbeats; every couple of minutes it glides down over Baross tér and
 * lands on the attic cornice above the great arch, folds its wings and
 * looks out over the square for half a minute, then drops off the ledge
 * and climbs back up to circle again.
 *
 * The game mode puts it at the middle of the station's main front, at
 * pavement level; everything it does is measured from there (the station's
 * frame: +X into the hall, east).
 */
UCLASS()
class PESTIALVILAG_API AKeletiEagle : public AActor
{
	GENERATED_BODY()

public:
	AKeletiEagle();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Eagle")
	TObjectPtr<USceneComponent> Root;

	/** The body (head and tail), turned nose-up when perched; the wings hang from it. */
	UPROPERTY(VisibleAnywhere, Category = "Eagle")
	TObjectPtr<UProceduralMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Eagle")
	TObjectPtr<UProceduralMeshComponent> WingLeft;

	UPROPERTY(VisibleAnywhere, Category = "Eagle")
	TObjectPtr<UProceduralMeshComponent> WingRight;

	/** Only shown when perched. */
	UPROPERTY(VisibleAnywhere, Category = "Eagle")
	TObjectPtr<UProceduralMeshComponent> Feet;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	enum class EFlight : uint8 { Circling, Landing, Perched, Leaving };

	/** Starts a glide along a curve from where it is now. */
	void StartGlide(EFlight Next, const FVector& Control1, const FVector& Control2, const FVector& End);
	/** Poses the wings (0 spread, 1 folded; Beat raises both tips, degrees) and the body (nose-up pitch). */
	void Pose(double Fold, double Beat, double BodyPitch);
	/** Faces along Direction, banking by Bank degrees (positive: right wing down). */
	void Face(const FVector& Direction, double Bank);
	FVector CirclePoint(double Angle) const;

	EFlight Flight = EFlight::Circling;
	/** Where the game put it: the middle of Keleti's front at pavement level. */
	FVector Home = FVector::ZeroVector;
	double CircleAngle = 0.0;
	/** Seconds left in the present state (circling, perched). */
	double StateTime = 0.0;
	/** Wingbeats: seconds to the next burst, and how long the present one lasts. */
	double BurstWait = 0.0;
	double BurstLeft = 0.0;
	double BeatPhase = 0.0;
	double Fold = 0.0;
	double LastYaw = 0.0;
	double PerchYaw = 180.0;
	double LookWait = 0.0;

	/** The glide: a cubic curve and how far along it is (0..1). */
	FVector Curve[4];
	double CurveT = 0.0;
	double CurveSeconds = 1.0;
	EFlight AfterGlide = EFlight::Circling;
};
