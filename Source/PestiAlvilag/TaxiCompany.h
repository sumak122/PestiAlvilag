// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TaxiCompany.generated.h"

class APestiTaxi;
class ATaxiPassenger;
class ATrafficManager;
class UMaterialInterface;
class UStaticMeshComponent;
class UTextRenderComponent;
class URouteGuideComponent;

enum class ETaxiShift : uint8
{
	Off,
	Waiting,      // on shift, the next fare comes soon
	ToPickup,     // a fare waits at the kerb
	ToDropoff,    // the fare rides in the taxi
};

/**
 * The Budapesti Autótaxi kiosk, where the player can work as a taxi driver.
 *
 * Walk up to the kiosk and press E to start a shift (E again ends it). On a
 * shift, drive one of the company's black and yellow taxis: a fare waves
 * from the kerb somewhere nearby under a yellow light. Stop beside them to
 * pick them up, then take them to the street they name and stop there. The
 * driver keeps a share of the meter, plus a tip for a quick, smooth ride.
 * Getting out, or the police coming after you, loses the fare.
 */
UCLASS()
class PESTIALVILAG_API ATaxiCompany : public AActor
{
	GENERATED_BODY()

public:
	ATaxiCompany();

	virtual void Tick(float DeltaSeconds) override;

	/** How close to the kiosk the player must stand to start or end a shift (cm). */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	float SignOnRange = 500.f;

	/** How close to a fare the taxi must stop (cm). */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	float PickupRange = 800.f;

	/** How close to the destination the taxi must stop (cm). */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	float DropoffRange = 1200.f;

	/** Below this the taxi counts as stopped (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	float StopSpeed = 150.f;

	/** A new fare waits this far from the taxi, in a straight line (cm). */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	float MinPickupDistance = 15000.f;

	UPROPERTY(EditAnywhere, Category = "Taxi")
	float MaxPickupDistance = 60000.f;

	/** Rides go this far, in a straight line (cm). */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	float MinRideDistance = 40000.f;

	UPROPERTY(EditAnywhere, Category = "Taxi")
	float MaxRideDistance = 150000.f;

	/** The meter: a flag fall plus so much per kilometre, in fillér (100 fillér = 1 pengő). */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	int32 FlagFall = 80;

	UPROPERTY(EditAnywhere, Category = "Taxi")
	int32 PerKilometre = 60;

	/** The driver's share of the meter; the company keeps the rest. Tips are the driver's. */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	float DriverShare = 0.5f;

	/** The speed a passenger expects along the streets (cm/s); faster earns a tip. */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	float ExpectedSpeed = 800.f;

	/** A waiting fare gives up after this many seconds. */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	float PatienceSeconds = 240.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Taxi")
	TArray<TObjectPtr<UStaticMeshComponent>> KioskParts;

	UPROPERTY(VisibleAnywhere, Category = "Taxi")
	TObjectPtr<UTextRenderComponent> SignFront;

	UPROPERTY(VisibleAnywhere, Category = "Taxi")
	TObjectPtr<UTextRenderComponent> SignBack;

	/** The yellow light over the destination. */
	UPROPERTY(VisibleAnywhere, Category = "Taxi")
	TObjectPtr<UStaticMeshComponent> Beacon;

	/** Arrows over the crossings on the way to the fare or the destination. */
	UPROPERTY(VisibleAnywhere, Category = "Taxi")
	TObjectPtr<URouteGuideComponent> Guide;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	virtual void BeginPlay() override;

private:
	void ToggleShift();
	void UpdateFare(APestiTaxi* Taxi, float DeltaSeconds);
	bool StartFare(const FVector& From);
	void PickUp(APestiTaxi* Taxi);
	void DropOff(APestiTaxi* Taxi, bool bPaid);
	void EndFare();
	/** A kerb spot on a named 1932 street between MinDistance and MaxDistance from From. */
	bool PickStreetSpot(const FVector& From, float MinDistance, float MaxDistance, FVector& OutKerb, FVector& OutRoad, FString& OutStreet) const;
	void Say(const FString& Line) const;
	ATrafficManager* GetTraffic() const;

	ETaxiShift Shift = ETaxiShift::Off;

	UPROPERTY(Transient)
	TObjectPtr<ATaxiPassenger> Passenger;

	FVector PickupSpot = FVector::ZeroVector;
	FString PickupStreet;
	FVector DropoffSpot = FVector::ZeroVector;
	FVector DropoffRoad = FVector::ZeroVector;
	FString DropoffStreet;
	double FareTime = 0.0;
	double NextFareTime = 0.0;
	float RideLength = 0.f;
	int32 BumpsAtPickup = 0;
	int32 BumpsSaid = 0;
	bool bPassengerScared = false;
	int32 Fares = 0;
};
