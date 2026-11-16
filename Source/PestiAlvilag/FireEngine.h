// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DailyLifeModel.h"
#include "FireEngine.generated.h"

class APedestrian;
class ATrafficManager;
class UBoxComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class USpotSound;

/**
 * The fire brigade's motor engine racing through the streets to a fire
 * (ADailyLife sends it, rarely): it follows a way along the streets (a list
 * of points) down the middle of the road, its bell clanging, the firemen in
 * their brass helmets on the bench. The traffic stops for it (a road block
 * just ahead of it, ATrafficManager::SetRoadBlock); it brakes for the
 * corners, bumps what it runs into and, held up for long, pushes on through.
 * Running into the player on foot knocks him over.
 */
UCLASS()
class PESTIALVILAG_API AFireEngine : public AActor
{
	GENERATED_BODY()

public:
	AFireEngine();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The way to go (city points, in order) and its manager, for the road block. Call once, after spawning. */
	void Setup(const TArray<FVector>& InWay, ATrafficManager* InTraffic);

	/** Got to the end of its way. */
	bool HasArrived() const { return Along >= Total; }

	/** Where along its way it is, and how much way there is (cm). */
	double GetAlong() const { return Along; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Fire")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Parts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APedestrian>> Crew;

	UPROPERTY(Transient)
	TObjectPtr<USpotSound> Bell;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	/** The fastest it may go now, for the corners coming. */
	float CornerLimit() const;
	double GroundAt(const FVector& At) const;
	/** Puts the firemen on their seats, rocking with the engine, and swings the bell. */
	void PlaceCrew(float DeltaSeconds);
	/** The point S along the way (no side offset), and the way's heading there. */
	FVector PointAt(double S, FVector& OutDir) const;

	TWeakObjectPtr<ATrafficManager> Traffic;
	TArray<FVector> Way;
	TArray<double> Cumulative;
	double Total = 0.0;
	double Along = 0.0;
	float Speed = 0.f;
	float BlockedTime = 0.f;
	float WheelTurn = 0.f;
	float BellSwing = 0.f;
	float Clock = 0.f;
	bool bReady = false;
};
