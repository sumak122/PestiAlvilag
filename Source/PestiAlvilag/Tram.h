// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarBody.h"
#include "GameFramework/Actor.h"
#include "Tram.generated.h"

class ATrafficManager;
class UBoxComponent;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * A tram that runs one route along its track, stops at the stops, and
 * brakes for anything on the rails in front of it, but never steers round
 * it. At the end of the route (the map edge) it leaves and comes back in at
 * the start a little later, as if it had gone round the city.
 */
UCLASS()
class PESTIALVILAG_API ATram : public AActor
{
	GENERATED_BODY()

public:
	ATram();

	virtual void Tick(float DeltaSeconds) override;

	/** Called by the manager right after spawning. */
	void StartOnRoute(ATrafficManager* InManager, int32 InRouteIndex, float InDistance);

	int32 GetRouteIndex() const { return RouteIndex; }
	float GetDistance() const { return Distance; }

	/** Standing at a stop with its doors open (and on the map). */
	bool IsAtStop() const { return DwellLeft > 0.f && OutOfServiceLeft <= 0.f; }
	/** The stop it stands at, or else the next one it will stop at (may be past the last). */
	int32 GetNextStop() const { return NextStop; }
	bool IsInService() const { return OutOfServiceLeft <= 0.f; }
	/** Keeps the doors open at least this long more (s). */
	void HoldAtStop(float Seconds) { if (IsAtStop()) { DwellLeft = FMath::Max(DwellLeft, Seconds); } }

	/** Cruising speed (cm/s); 700 is about 25 km/h. */
	UPROPERTY(EditAnywhere, Category = "Tram")
	float MaxSpeed = 700.f;

	/** Trams pick up speed slowly. */
	UPROPERTY(EditAnywhere, Category = "Tram")
	float Acceleration = 120.f;

	UPROPERTY(EditAnywhere, Category = "Tram")
	float BrakeDeceleration = 300.f;

	/** Seconds standing at each stop. */
	UPROPERTY(EditAnywhere, Category = "Tram")
	float DwellTime = 8.f;

	/** Centre-to-centre gap at which the tram stops behind something (cm). */
	UPROPERTY(EditAnywhere, Category = "Tram")
	float StopGap = 1000.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Tram")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, Category = "Tram")
	TArray<TObjectPtr<UStaticMeshComponent>> BodyParts;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	UPROPERTY(Transient)
	TObjectPtr<ATrafficManager> Manager;

	void PlaceOnTrack();
	void SetInService(bool bInService);
	bool IsStartClear() const;

	int32 RouteIndex = INDEX_NONE;
	float Distance = 0.f;
	float CurrentSpeed = 0.f;
	int32 NextStop = 0;
	float DwellLeft = 0.f;
	/** Seconds held up by an AI car; after a while the tram goes anyway. */
	float HeldUpTime = 0.f;
	/** Seconds until the tram comes back in at the start of its route. */
	float OutOfServiceLeft = 0.f;
	double GroundZ = 0.0;
	TArray<CarBody::EPaint> BodyPaints;
};
