// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PersonKit.h"
#include "TramModel.h"
#include "Tram.generated.h"

class ATrafficManager;
class UBoxComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;
class UTexture;

/**
 * A tram that runs one route along its track, stops at the stops, and
 * brakes for anything on the rails in front of it, but never steers round
 * it. At the end of the route (the map edge) it leaves and comes back in at
 * the start a little later, as if it had gone round the city.
 *
 * The body is TramModel's: open platforms at the ends and a saloon with
 * wooden benches, where a few passengers sit (and the player, riding).
 */
UCLASS()
class PESTIALVILAG_API ATram : public AActor
{
	GENERATED_BODY()

public:
	ATram();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

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

	/** A bench seat no passenger sits on (TramModel::Seat index), or INDEX_NONE. */
	int32 GetFreeSeat() const;

	/** Keeps a seat for the player while they ride (INDEX_NONE frees it). */
	void ReserveSeat(int32 Index) { ReservedSeat = Index; }

	/** How many people sit on the benches (not counting the player). */
	int32 NumPassengers() const { return TakenSeats.Num(); }
	/** Someone gets on and sits down; false if the benches are full. */
	bool SeatPassenger(const PersonKit::FOutfit& Outfit);
	/** Someone gets up to get off: they leave the bench, and OutOutfit is what they wear. */
	bool TakeOffPassenger(PersonKit::FOutfit& OutOutfit);

	/** The platform step nearest Point (on the ground beside the tram), where people get on and off. */
	FVector GetDoorNear(const FVector& Point) const;

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

	/** One mesh section per TramModel part. */
	UPROPERTY(VisibleAnywhere, Category = "Tram")
	TObjectPtr<UProceduralMeshComponent> Body;

	/** The seated passengers' legs, arms and bodies. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> PassengerPieces;

	/** Where each passenger sits (their middle), on the tram. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> PassengerSeats;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** The windows: the engine's see-through widget material. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

private:
	UPROPERTY(Transient)
	TObjectPtr<ATrafficManager> Manager;

	void BuildBody();
	void SeatPassengers();
	void AddPassenger(int32 SeatIndex, const PersonKit::FOutfit& Outfit);
	void RemovePassenger(int32 Which);
	void PlaceOnTrack();
	/** Knocks the player down if he stands in front of the moving tram. */
	void HitPlayerInTheWay();
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
	/** The seats the passengers sit on, and what each wears (PassengerPieces has NumPieces each). */
	TArray<int32> TakenSeats;
	TArray<PersonKit::FOutfit> PassengerOutfits;
	int32 ReservedSeat = INDEX_NONE;
};
